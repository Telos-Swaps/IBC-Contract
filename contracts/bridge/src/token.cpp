void bridge::addtoken( extended_symbol token_symbol, bool do_issue, asset min_quantity, name remote_chain, extended_symbol remote_token, bool enabled ) {
    require_auth( get_self() );

    tokens_table _tokens( get_self(), get_self().value );
    auto remote_token_index = _tokens.get_index<name("byremote")>();
    fees_t _fees_table( get_self(), get_self().value );

    // check token not already added
    check( _tokens.find( token_symbol.get_symbol().raw() ) == _tokens.end(), "token already exists");
    check( remote_token_index.find( remote_token.get_symbol().code().raw() ) == remote_token_index.end(), "remote token already exists");
    check( _fees_table.find( token_symbol.get_symbol().raw() ) == _fees_table.end(), "token already exists");

    _tokens.emplace( get_self(), [&]( auto& row ) {
        row.token_info = token_symbol;
        row.do_issue = do_issue;
        row.min_quantity = min_quantity;
        row.remote_chain = remote_chain;
        row.remote_token = remote_token;
        row.enabled = enabled;
    });

    _fees_table.emplace( get_self(), [&]( auto& row ) {
        row.total = asset(0, token_symbol.get_symbol());
        row.reserve = asset(0, token_symbol.get_symbol());
        row.last_distribution = current_time_point();
    });
}

void bridge::updatetoken( extended_symbol token_symbol, asset min_quantity, bool enabled ) {
    require_auth( get_self() );

    tokens_table _tokens( get_self(), get_self().value );
    auto token = _tokens.find( token_symbol.get_symbol().code().raw() );
    check( token != _tokens.end(), "token not found");

    _tokens.modify(token, get_self(), [&](auto &s) {
        s.enabled = enabled;
        s.min_quantity = min_quantity;
    });
}
/* Token fix patch
void bridge::fixtoken( extended_symbol token_symbol, extended_symbol remote_token ) {
    require_auth( get_self() );

    tokens_table _tokens( get_self(), get_self().value );
    auto token = _tokens.find( token_symbol.get_symbol().code().raw() );
    check( token != _tokens.end(), "token not found");

    _tokens.modify(token, get_self(), [&](auto &s) {
        s.token_info = token_symbol;
        s.remote_token = remote_token;
    });
}
*/