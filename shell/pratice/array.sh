#!/bin/sh

typeset -A my_array

my_array[0]=aaa
my_array[1]=bbb
my_array[2]=ccc
my_array["apple"]=33

echo ${my_array[0]}
echo ${my_array[1]}
echo ${my_array[2]}

echo ${my_array[*]}
echo ${my_array[@]}

echo ${#my_array[*]}
echo ${#my_array[@]}

echo ${my_array}
