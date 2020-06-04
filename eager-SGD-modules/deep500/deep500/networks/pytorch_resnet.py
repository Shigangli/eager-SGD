""" Adapted from https://github.com/eladhoffer/convNet.pytorch
    Reference:
    [1] Kaiming He, Xiangyu Zhang, Shaoqing Ren, Jian Sun
        Deep Residual Learning for Image Recognition. arXiv:1512.03385
"""

import math
import torch
import torch.nn as nn
from torch.autograd import Variable

def conv3x3(in_planes, out_planes, stride=1, groups=1, bias=False):
    """ 3x3 convolution with padding """
    return nn.Conv2d(in_planes, out_planes, kernel_size=3, stride=stride,
                     padding=1, groups=groups, bias=bias)


def init_model(model):
    for m in model.modules():
        if isinstance(m, nn.Conv2d):
            n = m.kernel_size[0] * m.kernel_size[1] * m.out_channels
            m.weight.data.normal_(0, math.sqrt(2. / n))
        elif isinstance(m, nn.BatchNorm2d):
            m.weight.data.fill_(1)
            m.bias.data.zero_()
    for m in model.modules():
        if isinstance(m, Bottleneck):
            nn.init.constant_(m.bn3.weight, 0)
        elif isinstance(m, BasicBlock):
            nn.init.constant_(m.bn2.weight, 0)

    model.fc.weight.data.normal_(0, 0.01)
    model.fc.bias.data.zero_()


class BasicBlock(nn.Module):

    def __init__(self, inplanes, planes,  stride=1, expansion=1,
                 downsample=None, groups=1, residual_block=None, dropout=0.):
        super(BasicBlock, self).__init__()
        dropout = 0 if dropout is None else dropout
        self.conv1 = conv3x3(inplanes, planes, stride, groups=groups)
        self.bn1 = nn.BatchNorm2d(planes)
        self.relu = nn.ReLU(inplace=True)
        self.conv2 = conv3x3(planes, expansion * planes, groups=groups)
        self.bn2 = nn.BatchNorm2d(expansion * planes)
        self.downsample = downsample
        self.residual_block = residual_block
        self.stride = stride
        self.expansion = expansion
        self.dropout = (nn.Dropout(dropout)
                        if dropout != 0.0 or dropout is not None
                        else lambda x: x)

    def forward(self, x):
        residual = x

        out = self.conv1(x)
        out = self.bn1(out)
        out = self.relu(out)
        out = self.dropout(out)

        out = self.conv2(out)
        out = self.bn2(out)

        if self.downsample is not None:
            residual = self.downsample(residual)

        if self.residual_block is not None:
            residual = self.residual_block(residual)

        out += residual
        out = self.relu(out)

        return out


class Bottleneck(nn.Module):

    def __init__(self, inplanes, planes,  stride=1, expansion=4,
                 downsample=None, groups=1, residual_block=None, dropout=0.):
        super(Bottleneck, self).__init__()
        dropout = 0 if dropout is None else dropout
        self.conv1 = nn.Conv2d(
            inplanes, planes, kernel_size=1, bias=False)
        self.bn1 = nn.BatchNorm2d(planes)
        self.conv2 = conv3x3(planes, planes, stride=stride, groups=groups)
        self.bn2 = nn.BatchNorm2d(planes)
        self.conv3 = nn.Conv2d(
            planes, planes * expansion, kernel_size=1, bias=False)
        self.bn3 = nn.BatchNorm2d(planes * expansion)
        self.relu = nn.ReLU(inplace=True)
        self.dropout = (nn.Dropout(dropout)
                        if dropout != 0.0 or dropout is not None
                        else lambda x: x)
        self.downsample = downsample
        self.residual_block = residual_block
        self.stride = stride
        self.expansion = expansion

    def forward(self, x):
        residual = x
        out = self.conv1(x)
        out = self.bn1(out)
        out = self.relu(out)
        out = self.dropout(out)

        out = self.conv2(out)
        out = self.bn2(out)
        out = self.relu(out)
        out = self.dropout(out)

        out = self.conv3(out)
        out = self.bn3(out)

        if self.downsample is not None:
            residual = self.downsample(residual)

        if self.residual_block is not None:
            residual = self.residual_block(residual)

        out += residual
        out = self.relu(out)

        return out


class ResNet(nn.Module):

    def __init__(self):
        super(ResNet, self).__init__()

    def _make_layer(self, block, planes, blocks, expansion=1, stride=1,
                    groups=1, residual_block=None, dropout=None):
        downsample = None
        out_planes = planes * expansion
        if stride != 1 or self.inplanes != out_planes:
            downsample = nn.Sequential(
                nn.Conv2d(self.inplanes, out_planes,
                          kernel_size=1, stride=stride, bias=False),
                nn.BatchNorm2d(planes * expansion),
            )
        if residual_block is not None:
            residual_block = residual_block(out_planes)

        layers = []
        layers.append(block(self.inplanes, planes, stride, expansion=expansion,
                            downsample=downsample, groups=groups,
                            residual_block=residual_block, dropout=dropout))
        self.inplanes = planes * expansion
        for i in range(1, blocks):
            layers.append(block(self.inplanes, planes, expansion=expansion,
                                groups=groups, residual_block=residual_block,
                                dropout=dropout))

        return nn.Sequential(*layers)

    def features(self, x):
        x = self.conv1(x)
        x = self.bn1(x)
        x = self.relu(x)
        x = self.maxpool(x)

        x = self.layer1(x)
        x = self.layer2(x)
        x = self.layer3(x)
        x = self.layer4(x)

        x = self.avgpool(x)
        return x.view(x.size(0), -1)

    def forward(self, x):
        x = self.features(x)
        x = self.fc(x)
        return x


class ResNet_imagenet(ResNet):

    def __init__(self, num_classes=1000, inplanes=64,
                 block=Bottleneck, residual_block=None, layers=[3, 4, 23, 3],
                 width=[64, 128, 256, 512], expansion=4, groups=[1, 1, 1, 1]):
        super(ResNet_imagenet, self).__init__()
        self.inplanes = inplanes
        self.conv1 = nn.Conv2d(3, self.inplanes, kernel_size=7, stride=2, padding=3,
                               bias=False)
        self.bn1 = nn.BatchNorm2d(self.inplanes)
        self.relu = nn.ReLU(inplace=True)
        self.maxpool = nn.MaxPool2d(kernel_size=3, stride=2, padding=1)

        for i in range(len(layers)):
            layer = self._make_layer(block=block, planes=width[i],
                                     blocks=layers[i], expansion=expansion,
                                     stride=1 if i == 0 else 2,
                                     residual_block=residual_block,
                                     groups=groups[i])
            setattr(self, 'layer%s' % str(i + 1), layer)

        self.avgpool = nn.AdaptiveAvgPool2d(1)
        self.fc = nn.Linear(width[-1] * expansion, num_classes)

        init_model(self)


class ResNet_cifar(ResNet):

    def __init__(self, num_classes=10, inplanes=16,
                 block=BasicBlock, depth=18, width=[16, 32, 64],
                 groups=[1, 1, 1], residual_block=None,
                 dropout=None):
        super(ResNet_cifar, self).__init__()
        self.inplanes = inplanes
        n = int((depth - 2) / 6)
        self.conv1 = nn.Conv2d(3, self.inplanes, kernel_size=3, stride=1, padding=1,
                               bias=False)
        self.bn1 = nn.BatchNorm2d(self.inplanes)
        self.relu = nn.ReLU(inplace=True)
        self.maxpool = lambda x: x

        self.layer1 = self._make_layer(block, width[0], n, groups=groups[
                                       0], residual_block=residual_block,
                                       dropout=dropout)
        self.layer2 = self._make_layer(
            block, width[1], n, stride=2, groups=groups[1],
            residual_block=residual_block, dropout=dropout)
        self.layer3 = self._make_layer(
            block, width[2], n, stride=2, groups=groups[2],
            residual_block=residual_block, dropout=dropout)
        self.layer4 = lambda x: x
        self.avgpool = nn.AvgPool2d(8)
        self.fc = nn.Linear(width[-1], num_classes)

        init_model(self)


def ResNet18(num_classes=10, in_channels=3):
    return ResNet_imagenet(num_classes=num_classes, block=BasicBlock,
                           layers=[2, 2, 2, 2], expansion=1)

def ResNet20(num_classes=10, in_channels=3):
    return ResNet_cifar(num_classes=num_classes, block=BasicBlock, depth=20)

def ResNet32(num_classes=10, in_channels=3):
    return ResNet_cifar(num_classes=num_classes, block=BasicBlock, depth=32)

def ResNet34(num_classes=10, in_channels=3):
    return ResNet_imagenet(num_classes=num_classes, block=BasicBlock,
                           layers=[3, 4, 6, 3], expansion=1)

def ResNet44(num_classes=10, in_channels=3):
    return ResNet_cifar(num_classes=num_classes, block=BasicBlock, depth=44)

def ResNet50(num_classes=10, in_channels=3):
    return ResNet_imagenet(num_classes=num_classes, block=Bottleneck,
                           layers=[3, 4, 6, 3])

def ResNet56(num_classes=10, in_channels=3):
    return ResNet_cifar(num_classes=num_classes, block=BasicBlock, depth=56)

def ResNet101(num_classes=10, in_channels=3):
    return ResNet_imagenet(num_classes=num_classes, block=Bottleneck,
                           layers=[3, 4, 23, 3])

def ResNet110(num_classes=10, in_channels=3):
    return ResNet_cifar(num_classes=num_classes, block=BasicBlock, depth=110)

def ResNet152(num_classes=10, in_channels=3):
    return ResNet_imagenet(num_classes=num_classes,
                           block=Bottleneck, layers=[3, 8, 36, 3])

def ResNet200(num_classes=10, in_channels=3):
    return ResNet_imagenet(num_classes=num_classes,
                           block=Bottleneck, layers=[3, 24, 36, 3])


_DEPTH_TO_FUNCTION = {18: ResNet18, 20: ResNet20, 32: ResNet32, 34: ResNet34,
                      44: ResNet44, 50: ResNet50, 56: ResNet56, 101: ResNet101,
                      110: ResNet110, 152: ResNet152, 200: ResNet200}


def export_resnet(batch_size, depth=50, classes=10, file_path='resnet.onnx',
                  shape=(3, 32, 32)):
    if depth not in _DEPTH_TO_FUNCTION:
        raise ValueError('ResNet depth %d not defined' % depth)
    
    net = _DEPTH_TO_FUNCTION[depth](classes, shape[0])
    dummy_input = Variable(torch.zeros((batch_size, *shape)))

    torch.onnx.export(net, dummy_input, file_path, verbose=False)

    # Free memory
    del net
    torch.cuda.empty_cache()

    return file_path
