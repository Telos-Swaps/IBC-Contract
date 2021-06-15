#! /bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

# contract
if [ -n "$1" ]; then
  contract=$1
  if [ -d "./contracts/$contract" ]; then
    echo -e "${GREEN}>>> Building $contract contract...${NC}"
  else
    echo -e "${RED}Error: Directory ./contracts/$contract does not exists.${NC}"
    exit 0
  fi
else
  echo -e "${RED}Error: contract needed, use ./build.sh <contract name>${NC}"
  exit 0
fi

mkdir -p build
mkdir -p build/$contract

eosio-cpp -I="./contracts/$contract/include" -R="./contracts/$contract/resources" -o="./build/$contract/$contract.wasm" -contract="$contract" -abigen ./contracts/$contract/$contract.cpp
