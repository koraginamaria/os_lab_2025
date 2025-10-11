#!/bin/bash

if [ $# -eq 0 ]; then
    echo "No arguments provided"
    exit 1
fi


count=$#
sum=0


for num in "$@"; do
    sum=$((sum + num))
done


average=$((sum / count))


echo "Count: $count"
echo "Average: $average"
