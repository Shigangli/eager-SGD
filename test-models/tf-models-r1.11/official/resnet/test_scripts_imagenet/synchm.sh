#!/bin/bash


for((i=1;i<=63;i++));
do
rm ./$i/* -rf
done

for((i=1;i<=63;i++));
do
cp ./0/* ./$i/ -r
done


echo "model synchronized"
