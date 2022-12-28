#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include "eosram.hpp"

namespace eosio {

class [[eosio::contract]] eosram : public contract {

public:
using contract::contract;

[[eosio::action]]
void reg(name miner){
    require_auth(miner);

      asset liquidity_balance = token::get_balance(extended_symbol(symbol("BOXCBJ", 0), name("lptoken.defi")),miner);
    
      token::registry regtable( "exominetoken"_n, "exominetoken"_n.value );
    
      auto minerid = regtable.find(miner.value);
      if( minerid == regtable.end() )
      {
          regtable.emplace( "exominetoken"_n, [&]( auto& row ) {
          row.miner = miner; 
        });
      } else {
          check(miner.to_string() != miner.to_string(), "This miner is already registered");
      }
}

template <typename T>
bool erase_table( T& table )
{
    auto itr = table.begin();
    bool erased = false;
    while ( itr != table.end() ) {
        itr = table.erase( itr );
        erased = true;
    }
    return erased;
}

[[eosio::action]]
void reset( const name table, const name acc )
{
    require_auth( get_self() );

    token::accounts _accs( get_self(), acc.value );

    if ( table == "accounts"_n ) erase_table( _accs );
    
    else check( false, "invalid table name");
}

[[eosio::action]]
void updatevalids(){
    require_auth(get_self());

    token::registry minerstable( "exominetoken"_n, "exominetoken"_n.value );
    token::validminers valids( "exominetoken"_n, "exominetoken"_n.value );
      for(auto itr = minerstable.begin();itr != minerstable.end();itr++){

          name validminer = itr->miner;

          asset liquidity_balance = token::update_lp_balance(extended_symbol(symbol("BOXCBJ", 0), name("lptoken.defi")),itr->miner);

          if (liquidity_balance.amount > 0) {
            
            valids.emplace( "exominetoken"_n, [&]( auto& row ) {
              row.miner = validminer;
              row.liquidity_balance = liquidity_balance;
              row.last_mine_time = current_time_point().sec_since_epoch();
            });
          }
    }
}

[[eosio::action]]
void mine(name miner){
    require_auth(miner);
    
    token::validminers _valids( "exominetoken"_n, "exominetoken"_n.value );

    auto ac = _valids.find(miner.value);
    if( ac == _valids.end() ) check(miner.to_string() != miner.to_string(), "You need to call first the REG action and wait at least 5 minutes before start mining");

    uint32_t last_mine = token::get_last_mine(miner);
    
    check(last_mine < current_time_point().sec_since_epoch(), "You are mining too fast");

    asset liquidity_balance = token::get_lp_balance(name("lptoken.defi"),miner, symbol_code("BOXCBJ"));
    asset new_balance = token::update_lp_balance(extended_symbol(symbol("BOXCBJ", 0), name("lptoken.defi")),miner);

    token::validminers valids( "exominetoken"_n, "exominetoken"_n.value );
    if (new_balance.amount < liquidity_balance.amount) {
      auto minitr1 = valids.find(miner.value);
      if (minitr1 == valids.end()) {
        check(miner.to_string() != miner.to_string(), "Account NOT found as valid miner");
      } else {
        valids.modify( minitr1, "exominetoken"_n, [&]( auto& row ) {
          row.miner = miner;
          row.liquidity_balance = new_balance;
          row.last_mine_time = current_time_point().sec_since_epoch();
        });
      }
    } else {
      auto minitr2 = valids.find(miner.value);
      if (minitr2 == valids.end()) {
        check(miner.to_string() != miner.to_string(), "Account NOT found as valid miner");
      } else {
        valids.modify( minitr2, "exominetoken"_n, [&]( auto& row ) {
          row.miner = miner;
          row.liquidity_balance = liquidity_balance;
          row.last_mine_time = current_time_point().sec_since_epoch();
        });
      }
    }

    uint64_t liquidity_amount = liquidity_balance.amount * 0.01;
    
    asset mine_amount = asset(liquidity_amount,symbol("EXO", 8));
    
    action(
      permission_level{"exominetoken"_n, "active"_n},
      name("exominetoken"),
      "issue"_n, 
      std::make_tuple(name("exominetoken"),mine_amount,std::string("issued ") + mine_amount.to_string() + std::string(" for: ") + miner.to_string())
    ).send();

    action(
      permission_level{"exominetoken"_n, "active"_n},
      name("exominetoken"),
      "transfer"_n, 
      std::make_tuple(name("exominetoken"),miner,mine_amount,std::string("Successfully mined: ") + mine_amount.to_string() + std::string(" by: ") + miner.to_string())
    ).send();
   }  
};

void token::create( const name&   issuer,
                    const asset&  maximum_supply )
{
    require_auth( get_self() );

    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( maximum_supply.is_valid(), "invalid supply");
    check( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( get_self(), [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void token::issue( const name& to, const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;
    check( to == st.issuer, "tokens can only be issued to issuer account" );

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must issue positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );
}

void token::retire( const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must retire positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void token::transfer( const name&    from,
                      const name&    to,
                      const asset&   quantity,
                      const string&  memo )
{
    check( from != to, "cannot transfer to self" );

    if ( from == "swap.defi"_n && memo == "Defibox: withdraw") {
        check( false, "Cannot withdraw LP tokens during Genesis Period (till EOS-EXO pair liquidity reaches 10k EOS)");
    }
  
    require_auth( from );
    check( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.code();
    stats statstable( get_self(), sym.raw() );
    const auto& st = statstable.get( sym.raw(), "no balance with specified symbol" );

    require_recipient( from );
    require_recipient( to );

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity );
    add_balance( to, quantity, payer );
}

void token::sub_balance( const name& owner, const asset& value ) {
   accounts from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
}

void token::add_balance( const name& owner, const asset& value, const name& ram_payer )
{
   accounts to_acnts( get_self(), owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void token::open( const name& owner, const symbol& symbol, const name& ram_payer )
{
   require_auth( ram_payer );

   check( is_account( owner ), "owner account does not exist" );

   auto sym_code_raw = symbol.code().raw();
   stats statstable( get_self(), sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
   check( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}

void token::close( const name& owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}
} /// namespace eosio
