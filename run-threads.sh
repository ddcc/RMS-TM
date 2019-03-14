#!/bin/bash

#set -e

if [ $# -ne 2 ]; then
  echo "$0 <input> <iterations>"
  exit
fi

export ITM_STATISTICS=verbose
export LD_PRELOAD=../tinySTM/abi/gcc/libitm.so

for i in 1 2 4 8 16 32 64; do
  for ((j=1; j<=${2}; j++)); do
    echo "Running $i threads at iteration $j"
    if [ -f "${1}/APR/offset_file_4000_20_1_P${i}.txt" ]; then
      ./src/MineBench/Apriori/no_output_apriori -i ${1}/APR/data.ntrans_4000.tlen_20.nitems_1.npats_2000.patlen_6 -f ${1}/APR/offset_file_4000_20_1_P${i}.txt -s 0.0075 -n ${i} >> apr.$i.log
    fi
    ./src/MineBench/ScalParC/scalparc ${1}/ScalParC/para_F26-A64-D250K/F26-A64-D250K.tab 250000 64 2 ${i} >> scalparc.$i.log
    if [ -f "${1}/utility_mine/GEN/offset_2000_20_1/offset_2000_20_1_6_P${i}.txt" ]; then
      ./src/MineBench/UtilityMine/utility_mine ${1}/utility_mine/GEN/data.ntrans_2000.tlen_20.nitems_1.patlen_6 ${1}/utility_mine/GEN/offset_2000_20_1/offset_2000_20_1_6_P${i}.txt ${1}/utility_mine/GEN/logn2000_binary 0.01 ${i} >> utilitymine.$i.log
    fi
  done
done
