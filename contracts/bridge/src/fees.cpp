void bridge::issuefees() {
    settings_t _settings_table( get_self(), get_self().value );
    auto _settings = _settings_table.get();
    fees_t _fees_table( get_self(), get_self().value );
    reporters_t _reporters_table( get_self(), get_self().value );
    tokens_table _tokens_table( get_self(), get_self().value );

    check_enabled(); // can be called by anyone

    uint64_t total_points = 0;

    for (auto reporter = _reporters_table.begin();
         reporter != _reporters_table.end(); reporter++) {
        total_points += reporter->points;
    }

    // wait until ~10 transfers have been processed
    // 1 transfer needs at least threshold reports + 1 execute
    check(total_points > (_settings.threshold + 1) * 10, "not enough transfers have been processed since last time");

    // iterate through all tokens on bridge
    for ( auto itr = _fees_table.cbegin(); itr != _fees_table.cend(); itr++ ) {
        auto _fees = *itr;
        asset reserve = _fees.reserve;
        asset distributed = asset(0, reserve.symbol);

        auto _token = _tokens_table.find( reserve.symbol.code().raw() );
        check( _token != _tokens_table.end(), "token not found" );

        for (auto reporter = _reporters_table.begin();
             reporter != _reporters_table.end(); reporter++) {
            double share = (double)reporter->points / total_points;
            asset share_fees = asset(share * reserve.amount, reserve.symbol);

            distributed += share_fees;

            _reporters_table.modify(reporter, eosio::same_payer, [&](auto &s) {
                // reset points
                s.points = 0;
            });

            if (share_fees.amount > 0) {
                token::transfer_action transfer_act(_token->token_info.get_contract(), {get_self(), name("active")});
                transfer_act.send(get_self(), reporter->account, share_fees, "fees");
            }
        }

        // should be close to 0, roll over dust
        _fees.reserve -= distributed;
        check(_fees.reserve.amount > 0, "negative reserve, something went wrong");
        _fees.last_distribution = current_time_point();

        _fees_table.modify(itr, get_self(), [&](auto &s) {
            s = _fees;
        });
    }
}

void bridge::reporter_worked(const name &reporter) {
    reporters_t _reporters_table( get_self(), get_self().value );

    auto it = _reporters_table.find(reporter.value);
    check(it != _reporters_table.end(), "reporter does not exist while PoW");

    _reporters_table.modify(it, eosio::same_payer, [&](auto &s) {
        s.points++;
    });
}
