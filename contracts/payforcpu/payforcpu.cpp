#include <eosio/eosio.hpp>

using namespace eosio;

class [[eosio::contract("payforcpu")]] payforcpunet : public contract {
public:
    using contract::contract;

    [[eosio::action]]
    void payforcpu() {
        require_auth( get_self() );
    }
};