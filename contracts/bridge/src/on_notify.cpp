void bridge::on_transfer(name from, name to, asset quantity, string memo) {
    settings_t _settings_table( get_self(), get_self().value );
    auto _settings = _settings_table.get();
    tokens_table _tokens_table( get_self(), get_self().value );

    check_enabled();

    if (from == get_self() || from == "eosio.ram"_n || from == "eosio.stake"_n || from == "eosio.rex"_n) return;

    auto _token = _tokens_table.get( quantity.symbol.code().raw(), "token not found" );
    check(to == get_self(), "contract not involved in transfer");
    check(get_first_receiver() == _token.token_info.get_contract(),"incorrect token contract");
    check(quantity.symbol == _token.token_info.get_symbol(), "correct token contract, but wrong symbol");
    check(quantity >= _token.min_quantity, "sent quantity is less than required min quantity");

    const memo_x_transfer &memo_object = parse_memo(memo);

    std::string to_blockchain(memo_object.to_blockchain);
    std::transform(to_blockchain.begin(), to_blockchain.end(), to_blockchain.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    name to_blockchain_name = name(to_blockchain);

    check(to_blockchain_name == name("eos") || to_blockchain_name == name("telos"),
          "invalid memo: target blockchain \"" + to_blockchain + "\" is not valid");
    check(_settings.current_chain_name != to_blockchain_name,"cannot send to the same chain");
    check(memo_object.to_account.size() > 0 && memo_object.to_account.size() < 13,
          "invalid memo: target name \"" + memo_object.to_account + "\" is not valid");

    register_transfer(to_blockchain_name, from, name(memo_object.to_account), quantity, memo_object.memo, false);
}
