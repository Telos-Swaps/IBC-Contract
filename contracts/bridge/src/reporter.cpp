void bridge::addreporter(name reporter) {
    reporters_t _reporters_table( get_self(), get_self().value );

    require_auth(get_self());
    check(is_account(reporter), "reporter account does not exist");
    auto it = _reporters_table.find(reporter.value);

    check(it == _reporters_table.end(), "reporter already defined");

    _reporters_table.emplace(get_self(), [&](auto &s) {
        s.account = reporter;
        s.points = 0;
    });
}

void bridge::rmreporter(name reporter) {
    reporters_t _reporters_table( get_self(), get_self().value );

    require_auth(get_self());
    auto it = _reporters_table.find(reporter.value);

    check(it != _reporters_table.end(), "reporter does not exist");

    _reporters_table.erase(it);
}
