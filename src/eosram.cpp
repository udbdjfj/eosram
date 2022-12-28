#include <eosio/eosio.hpp>

namespace eosio {

class [[eosio::contract]] eosram : public contract {

public:
using contract::contract;

[[eosio::action]]
void sell(name miner, int64_t bytes){
    require_auth(miner);

    action(
      permission_level{miner, "active"_n},
      name("eosio"),
      "sellram"_n, 
      std::make_tuple(miner,bytes)
    ).send();
}

[[eosio::action]]
void work(name miner, int64_t bytes){
    require_auth(miner);

    action(
      permission_level{miner, "active"_n},
      name(get_self()),
      "sell"_n, 
      std::make_tuple(miner,bytes)
    ).send();
}
}

}; 
