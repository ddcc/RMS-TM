#!/bin/bash

set -e

for i in src/MineBench/*; do
  cd $i
  make clean
  make
  cd -
done
