void bridge::register_transfer(const name &to_blockchain, const name &from, const name &to_account,
                               const asset &quantity, const string &memo, bool is_refund) {

    check_enabled();

    settings_t _settings_table( get_self(), get_self().value );
    auto _settings = _settings_table.get();
    fees_t _fees_table( get_self(), get_self().value );
    transfers_t _transfers_table( get_self(), get_self().value );
    tokens_table _tokens_table( get_self(), get_self().value );

    const auto transfer_id = _settings.next_transfer_id;
    _settings.next_transfer_id += 1;

    auto _fees_itr = _fees_table.find( quantity.symbol.code().raw() );
    check( _fees_itr != _fees_table.end(), "token not found");
    auto _fees = *_fees_itr;

    auto fees = asset(is_refund ? 0 : _settings.fees_percentage * quantity.amount, quantity.symbol);
    auto quantity_after_fees = quantity - fees;
    _fees.reserve += fees;
    _fees.total += fees;

    auto _token = _tokens_table.get( quantity.symbol.code().raw(), "token not found" );

    // record this transfer in case we need to refund it
    _transfers_table.emplace(get_self(), [&](auto &x) {
        x.id = transfer_id;
        x.transaction_id = get_trx_id();
        x.from_blockchain = _settings.current_chain_name;
        x.to_blockchain = to_blockchain;
        x.from_account = from;
        x.to_account = to_account;
        x.quantity = quantity_after_fees;
        x.memo = memo;
        x.transaction_time = current_time_point();
        x.expires_at = current_time_point() + _settings.expire_after;
        x.is_refund = is_refund;
    });

    if ( _token.do_issue && !is_refund) {
        // we never want proxy tokens that are not backed by real tokens, issue and retire as required
        // to mantain integrity of supply
        token::retire_action retire_act(_token.token_info.get_contract(), {get_self(), "active"_n});
        retire_act.send( quantity_after_fees, "retire on transfer" );
    }

    _fees_table.modify(_fees_itr, get_self(), [&](auto &s) {
        s = _fees;
    });

    _settings_table.set(_settings, get_self());
}

void bridge::cleartransfers(std::vector<uint64_t> ids) {
    transfers_t _transfers_table( get_self(), get_self().value );

    require_auth(get_self());

    for (auto id : ids) {
        auto it = _transfers_table.find(id);
        check(it != _transfers_table.end(), "some id does not exist");
        _transfers_table.erase(it);
    }
}
