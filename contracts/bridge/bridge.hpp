#pragma once

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract("bridge")]] bridge : public contract {
public:
    using contract::contract;

    /**
     * Metadata about the contract and project
     */
    struct [[eosio::table("info.row")]] info_row {
        name                        key;
        string                      value;

        uint64_t primary_key() const { return key.value; }
    };
    typedef eosio::multi_index< "info"_n, info_row > info_table;

    /*
     *
     */
    struct [[eosio::table("settings")]] settings {
        name admin_account;
        name current_chain_name;
        bool enabled;
        eosio::microseconds expire_after = days(7); // duration after which transfers and reports expire can be evicted from RAM
        uint8_t threshold;                          // how many reporters need to report a transfer for it to be confirmed
    };
    typedef eosio::singleton<"settings"_n, settings> settings_singleton;

    /*
     *
     */
    struct [[eosio::table("channels")]] channels {
        name channel_name;
        name remote_contract;
        uint64_t next_transfer_id = 0;
        bool enabled = true;

        uint64_t primary_key()const { return channel_name.value; }
    };
    typedef eosio::multi_index< "channels"_n, channels > channels_table;

    /*
     *
     */
    struct [[eosio::table("tokens")]] tokens {
        name channel;
        extended_symbol token_info;                  // stores info about the token that needs to be issued
        bool do_issue;                               // whether tokens should be issued or taken from the contract balance
        asset min_quantity;                          // minimum transfer quantity
        extended_symbol remote_token;
        bool enabled;

        uint64_t primary_key() const { return token_info.get_symbol().code().raw(); }
        uint64_t by_remote() const { return remote_token.get_symbol().code().raw(); }
    };
    typedef eosio::multi_index<"tokens"_n, tokens,
            indexed_by<"byremote"_n, const_mem_fun<tokens, uint64_t, &tokens::by_remote>>>
    tokens_table;

    /*
     *
     */
    struct [[eosio::table("transfer")]] transfer_s {
        uint64_t id; // tokens_table.next_transfer_id
        checksum256 transaction_id;
        name from_blockchain;
        name to_blockchain;
        name from_account;
        name to_account;
        asset quantity;
        string memo;
        time_point_sec transaction_time;
        time_point_sec expires_at;                  // secondary index
        bool is_refund = false;                     // refunds are implemented as separate transfers

        uint64_t primary_key() const { return id; }
        uint64_t by_expiry() const { return _by_expiry(*this); }
        static uint64_t _by_expiry(const transfer_s &t) { return t.expires_at.sec_since_epoch();}

        bool operator==(const transfer_s &b) const {
            auto a = *this;
            return a.id == b.id && a.transaction_id == b.transaction_id &&
                   a.from_blockchain == b.from_blockchain &&
                   a.to_blockchain == b.to_blockchain &&
                   a.from_account == b.from_account && a.to_account == b.to_account &&
                   a.quantity == b.quantity &&
//                  a.memo == b.memo && // ignore memo for now
                   a.transaction_time == b.transaction_time &&
                   a.expires_at == b.expires_at && a.is_refund == b.is_refund;
        }
    };
    typedef eosio::multi_index<"transfers"_n, transfer_s,
            indexed_by<"byexpiry"_n, const_mem_fun<transfer_s, uint64_t, &transfer_s::by_expiry>>
    > transfers_t;

    /*
     *
     */
    struct [[eosio::table("report")]] report_s {
        uint64_t id;
        transfer_s transfer;
        bool confirmed = false;
        std::vector<name> confirmed_by;
        bool executed = false;
        bool failed = false;
        std::vector<name> failed_by;

        uint64_t primary_key() const { return id; }
        uint128_t by_transfer_id() const { return _by_transfer_id(transfer); }
        static uint128_t _by_transfer_id(const transfer_s &t) {
            return static_cast<uint128_t>(t.from_blockchain.value) << 64 | t.id;
        }
        uint64_t by_expiry() const { return transfer_s::_by_expiry(transfer); }
    };
    typedef eosio::multi_index<"reports"_n, report_s,
            indexed_by<"bytransferid"_n, const_mem_fun<report_s, uint128_t, &report_s::by_transfer_id>>,
            indexed_by<"byexpiry"_n, const_mem_fun<report_s, uint64_t, &report_s::by_expiry>>
        > reports_t;
    // keeps track of unprocessed reports that expired for manual review
    typedef eosio::multi_index<"reports.expr"_n, report_s> expired_reports_t;

    /*
     *
     */
    struct [[eosio::table]] reporter_info {
        name account;
        uint64_t points = 0;

        uint64_t primary_key() const { return account.value; } // need to serialize public_key
        EOSLIB_SERIALIZE(reporter_info, (account)(points))
    };
    typedef eosio::multi_index<"reporters"_n, reporter_info> reporters_t;

    /**
     *
     * @param admin_account
     * @param current_chain_name
     * @param expire_after_seconds
     * @param threshold
     */
    [[eosio::action]]
    void init( name admin_account, name current_chain_name, uint32_t expire_after_seconds, uint8_t threshold );

    [[eosio::action]]
    void addchannel( name& channel_name, name& remote_contract );

    /**
     *
     * @param channel
     * @param token_symbol
     * @param do_issue
     * @param min_quantity
     * @param remote_token
     * @param enabled
     */
    [[eosio::action]]
    void addtoken( const name &channel, extended_symbol token_symbol, bool do_issue, asset min_quantity, extended_symbol remote_token, bool enabled );

    /**
     *
     * @param channel
     * @param token_symbol
     * @param min_quantity
     * @param enabled
     */
    [[eosio::action]]
    void updatetoken( const name& channel, const extended_symbol& token_symbol, const asset& min_quantity, const bool& enabled );

    /**
     *
     * @param channel
     * @param expire_after_seconds
     * @param threshold
     */
    [[eosio::action]]
    void update( const name& channel, const uint32_t& expire_after_seconds, const uint64_t& threshold );

    /**
     *
     * @param enable
     */
    [[eosio::action]]
    void enable(bool enable);

    /**
     *
     * @param reporter
     */
    [[eosio::action]]
    void addreporter(name reporter);

    /**
     *
     * @param reporter
     */
    [[eosio::action]]
    void rmreporter(name reporter);

    /**
     *
     * @param channel_name
     * @param ids
     */
    [[eosio::action("clear.trans")]]
    void cleartransfers( const name& channel_name, std::vector<uint64_t> ids );

    /**
     *
     * @param channel
     * @param ids
     */
    [[eosio::action("clear.rep")]]
    void clearreports( const name& channel, std::vector<uint64_t> ids );

    /**
     *
     * @param channel
     * @param count
     */
    [[eosio::action("clear.exp")]]
    void clearexpired( const name &channel, const uint64_t &count );

    /**
     *
     * @param reporter
     * @param channel
     * @param transfer
     */
    [[eosio::action]]
    void report( const name& reporter, const name& channel, const transfer_s& transfer );

    /**
     *
     * @param reporter
     * @param channel
     * @param report_id
     */
    [[eosio::action]]
    void exec( const name& reporter, const name& channel, const uint64_t& report_id );

    /**
     *
     * @param reporter
     * @param channel
     * @param report_id
     */
    [[eosio::action]]
    void execfailed( const name& reporter, const name& channel, const uint64_t& report_id );

    /**
     *
     * @param from
     * @param to
     * @param quantity
     * @param memo
     */
    [[eosio::on_notify("*::transfer")]]
    void on_transfer( const name       from,
                      const name       to,
                      const asset      quantity,
                      const string     memo );

private:
    settings get_settings();

    bool channel_exists( name channel_name );

    void register_transfer( const name& channel_name, const name& from, const name& to_account,
                            const asset& quantity, const string& memo, bool is_refund );
    void reporter_worked(const name& reporter);
    void free_ram( const name& channel );

    checksum256 get_trx_id() {
        size_t size = transaction_size();
        char buf[size];
        size_t read = read_transaction(buf, size);
        check(size == read, "read_transaction failed");
        return sha256(buf, read);
    }

    struct memo_x_transfer {
        string version;
        string to_blockchain;
        string to_account;
        string memo;
    };

    memo_x_transfer parse_memo( string memo ) {
        string to_memo = "";
        if (memo.find("|") != string::npos) {
            auto major_parts = split(memo, "|"); // split the memo by "|"
            to_memo = memo;
            to_memo.erase( 0, major_parts[0].length() + 1 );
            memo = major_parts[0];
        }

        auto res = memo_x_transfer();
        auto parts = split(memo, "@");
        res.version = "2.0";
        res.to_blockchain = parts[1];
        res.to_account = parts[0];
        res.memo = to_memo;
        return res;
    }

    void check_reporter(name reporter) {
        reporters_t _reporters_table( get_self(), get_self().value );
//        auto existing = _reporters_table.find(reporter.value);

//        check(existing != _reporters_table.end(), "the signer is not a known reporter");
        check( _reporters_table.find(reporter.value) != _reporters_table.end(), "the signer is not a known reporter" );
    }

    /**
     * TODO this function is hardcoded for EOS / TLOS, change to allow remote chain and contract to be configured
     * TODO is this necessary? Seems like reporter works anyway
     * @param chain_name
     * @return
     */
    name get_ibc_contract_for_channel(name channel) {
        channels_table channels( get_self(), get_self().value );
        auto channel_itt = channels.find( channel.value );
        check( channel_itt != channels.end(), "unknown channel, \"" + channel.to_string() + "\"" );
        check( channel_itt->enabled, "channel disabled");

        return channel_itt->remote_contract;
    }

    uint32_t get_num_reporters() {
        uint32_t count = 0;
        reporters_t _reporters_table( get_self(), get_self().value );
        for (auto it = _reporters_table.begin(); it != _reporters_table.end(); it++) {
            count++;
        }

        return count;
    }

    std::vector<std::string> split(const std::string& str, const std::string& delim) {
        std::vector<std::string> tokens;
        size_t prev = 0, pos = 0;

        do
        {
            pos = str.find(delim, prev);
            if (pos == std::string::npos) pos = str.length();
            std::string token = str.substr(prev, pos - prev);
            tokens.push_back(token);
            prev = pos + delim.length();
        }
        while (pos < str.length() && prev < str.length());
        return tokens;
    }

    void push_first_free(std::vector<eosio::name>& vec, eosio::name val) {
        for(auto it = vec.begin(); it != vec.end(); it++) {
            if(it->value == 0) {
                *it = val;
                return;
            }
        }
        eosio::check(false, "push_first_free: iterated past vector");
    }

    uint32_t count_non_empty(const std::vector<eosio::name>& vec) {
        uint32_t count = 0;
        for(auto it : vec) {
            if(it.value == 0) {
                break;
            }
            count++;
        }
        return count;
    }

};
