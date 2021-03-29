void bridge::init(name current_chain_name, uint32_t expire_after_seconds, uint8_t threshold, double fees_percentage) {

    settings_t _settings_table( get_self(), get_self().value );
    fees_t _fees_table( get_self(), get_self().value );

    require_auth(get_self());

    bool settings_exists = _settings_table.exists();

    check(!settings_exists, "settings already defined");
    check(threshold > 0, "threshold must be positive");

    _settings_table.set(
            settings{
                    .current_chain_name = current_chain_name,
                    .enabled = false,
                    .expire_after = seconds(expire_after_seconds),
                    .threshold = threshold,
            },
            get_self());
}

void bridge::update( uint32_t expire_after_seconds, uint64_t threshold, double fees_percentage ) {
    settings_t _settings_table( get_self(), get_self().value );
    auto _settings = _settings_table.get();
    reports_t _reports_table( get_self(), get_self().value );

    require_auth(get_self());

    check(threshold > 0, "minimum reporters must be positive");

    _settings.threshold = threshold;
    _settings.expire_after = seconds(expire_after_seconds);
    _settings.fees_percentage = fees_percentage;
    _settings_table.set(_settings, get_self());

    // need to update all unconfirmed reports and check if they are now confirmed
    for ( auto report = _reports_table.begin(); report != _reports_table.end(); report++ ) {
        if ( !report->confirmed && count_non_empty(report->confirmed_by) >= threshold ) {
            _reports_table.modify(report, eosio::same_payer, [&](auto &s) {
                s.confirmed = true;
            });
        }
    }
}

void bridge::enable(bool enable) {
    settings_t _settings_table( get_self(), get_self().value );
    auto _settings = _settings_table.get();

    require_auth(get_self());

    _settings.enabled = enable;
    _settings_table.set(_settings, get_self());
}
