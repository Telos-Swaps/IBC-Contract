#!/bin/bash

declare -a contracts=( "bridge" "payforcpu" )

cd ../contracts

for contract in "${contracts[@]}"
do
    cd ${contract}
    echo -e "${GREEN}Compiling $contract...${NC}"
    eosio-cpp -abigen -I include -o $contract.wasm $contract.cpp
    cd ..
done
