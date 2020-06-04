#ifndef CROSSCORRELATION_SWAP_AXES_HPP
#define CROSSCORRELATION_SWAP_AXES_HPP

#include <deque>
#include "../../tensor/indexing.hpp"
#include "../../tensor/set_all.hpp"

//crosscorrelation, axes 0 and 1 of X, W are swapped

namespace utilityMath {

template <class T>
void crosscorrelation_swap_axes(
    const std::deque<uint32_t>& input_shape,
    const std::deque<uint32_t>& output_shape,
    const size_t group,
    const std::deque<uint32_t>& weight_shape,
    const std::deque<uint32_t>& dilations,
    const std::deque<uint32_t>& pads,
    const std::deque<uint32_t>& strides,
    const T* input,
    const T* kernel,
    T* output) {

    //compute number of elements of output tensor
    int output_size = 1;
    for(const auto& elem: output_shape)
        output_size *= elem;

    //beyond this point: input-, output- and kernel dimensions, stride, padding, dilation must conform
    utilityTensor::set_all_zero(output_size, output);

    // H = D1
    const size_t D1 = input_shape[2];
    // W = D2
    const size_t D2 = input_shape[3];
    const size_t D3 = input_shape[4];
    const size_t c = weight_shape[0];
    const size_t kd1 = weight_shape[2];
    const size_t kd2 = weight_shape[3];
    const size_t kd3 = weight_shape[4];

    //loop over groups
    size_t input_temp_index;
    size_t kernel_temp_index;
    size_t output_temp_index;
    size_t j_output_temp;
    size_t t_input_temp;
    size_t i_output_temp;
    int k_input_temp;
    int l_input_temp;
    int m_input_temp;
    for(size_t g = 0; g < group; g++) {
        //loop over outputs tensor
        for(size_t i = 0; i < output_shape[1]; i++) {
            i_output_temp = i + g * output_shape[1];
            for(size_t j = 0; j < (output_shape[0] / group); j++) {
                j_output_temp = j + g * (output_shape[0] / group);
                for(size_t k = 0; k < output_shape[2]; k++) {
                    for(size_t l = 0; l < output_shape[3]; l++) {
                        for(size_t m = 0; m < output_shape[4]; m++) {
                            //loop over kernel
                            for(size_t t = 0; t < c; t++) {
                                t_input_temp = t + g * c;
                                for(size_t u = 0; u < kd1; u++) {
                                    k_input_temp = -pads[0] + k * strides[0] + u * dilations[0];
                                    if(k_input_temp >= 0 && k_input_temp < D1) {
                                        for(size_t v = 0; v < kd2; v++) {
                                            l_input_temp = -pads[1] + l * strides[1] + v * dilations[1];
                                            if(l_input_temp >= 0 && l_input_temp < D2) {
                                                for(size_t w = 0; w < kd3; w++) {  
                                                    m_input_temp = -pads[2] + m * strides[2] + w * dilations[2];
                                                    if(m_input_temp >= 0 && m_input_temp < D3) { 
                                                        input_temp_index = utilityTensor::compute_index(
                                                            input_shape,
                                                            {t,
                                                            i_output_temp,
                                                            static_cast<uint32_t>(k_input_temp),
                                                            static_cast<uint32_t>(l_input_temp),
                                                            static_cast<uint32_t>(m_input_temp)});
                                                        kernel_temp_index = utilityTensor::compute_index(
                                                            weight_shape,
                                                            {t,
                                                            j_output_temp,
                                                            u,
                                                            v,
                                                            w});
                                                        output_temp_index = utilityTensor::compute_index(
                                                            output_shape,
                                                            {j_output_temp,
                                                            i,
                                                            k,
                                                            l,
                                                            m});
                                                        output[output_temp_index] += kernel[kernel_temp_index] * input[input_temp_index];
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return;
};

}

#endif
