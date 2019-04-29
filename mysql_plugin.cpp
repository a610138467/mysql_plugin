#include <eosio/mysql_plugin/mysql_plugin.hpp>

#include <queue>

namespace eosio {

using std::queue;

static appbase::abstract_plugin& _mysql_relay_plugin = app().register_plugin<mysql_plugin>();

void mysql_plugin::set_program_options(options_description& cli, options_description& cfg) {
    cfg.add_options()
        ("mysql-plugin-enable", bpo::bool_switch()->default_value(false), "weather enable the mysql_plugin")
        ("mysql-plugin-host", bpo::value<string>()->default_value("127.0.0.1"),   "the mysql host")
        ("mysql-plugin-port", bpo::value<uint32_t>()->default_value(3306),  "the mysql port")
        ("mysql-plugin-user",    bpo::value<string>()->default_value("root"), "the mysql user")
        ("mysql-plugin-password", bpo::value<string>()->default_value("root"), "the mysql password")
        ("mysql-plugin-database", bpo::value<string>()->default_value("eosio"), "the myqsl database");
        ;
}

void mysql_plugin::plugin_initialize(const variables_map& options) { 
    enable = options.at("mysql-plugin-enable").as<bool>(); 
    string host = options.at("mysql-plugin-host").as<string>();
    uint32_t port = options.at("mysql-plugin-port").as<uint32_t>();
    string user = options.at("mysql-plugin-user").as<string>();
    string password = options.at("mysql-plugin-password").as<string>();
    string database = options.at("mysql-plugin-database").as<string>();

    if (!enable) return;
    if (host == "") {
        wlog ("mysql-plugin is enabled, but host is illegal, plugin will be disabled");
        enable = false;
        return;
    }
    if (port == 0) {
        wlog ("mysql-plugin is enabled, but port is illegal, plugin will be disabled");
        enable = false;
        return;
    }
    if (user == "") {
        wlog ("mysql-plugin is enabled, but user is illegal, plugin will be disabled");
        enable = false;
        return;
    }
    if (password == "") {
        wlog ("mysql-plugin is enabled, but password is illegal, plugin will be disabled");
        enable = false;
        return;
    }
    if (database == "") {
        wlog ("mysql-plugin is enabled, but database is illegal, plugin will be disabled");
        enable = false;
        return;
    }

    mysql_connection = mysql_init(NULL);
    if (mysql_connection == NULL) {
        elog ("init mysql connection failed");
        enable = false;
        return;
    }
    if (!mysql_real_connect(mysql_connection, host.c_str(), user.c_str(), password.c_str(), database.c_str(), port, NULL, 0)) {
        elog ("connect mysql server failed : ${error}", ("error", mysql_error(mysql_connection)));
        enable = false;
        return;
    }

    auto& chain = app().get_plugin<chain_plugin>().chain();
    on_applied_transaction_connection = chain.applied_transaction.connect([=](const transaction_trace_ptr& transaction_trace) {
        auto action_traces = transaction_trace->action_traces;
        queue<action_trace> walk_traces;
        for (auto trace : action_traces)
            walk_traces.push(trace);
        while (!walk_traces.empty()) {
            auto trace = walk_traces.front();
            analyse_action(trace);
            walk_traces.pop();
            auto inline_traces = trace.inline_traces;
            for (auto inline_trace : inline_traces) {
                walk_traces.push(inline_trace);
            }
        }
    });
}

void mysql_plugin::plugin_startup() {
    if (!enable) return;
    ilog ("mysql_plugin started");
}

void mysql_plugin::plugin_shutdown() {
    if (!enable) return;
    mysql_close(mysql_connection);
    ilog ("mysql_plugin shutdown");
}

void mysql_plugin::analyse_action (action_trace& trace) {
    string account = string(trace.act.account);
    string name = string(trace.act.name);
    string receiver = string(trace.receipt.receiver);
    if (account != receiver) return;
    auto trace_variant = app().get_plugin<chain_plugin>()
        .chain().to_variant_with_abi(trace, fc::seconds(10));
    //dlog (  "trace info : account : ${account} name : ${name} receiver : ${receiver} data : ${data}", 
    //        ("account", account)("name", name)("receiver", receiver)("data", data));
    //TODO: 这里分析action,目前得到了一个action合约名称(account)方法(name)和参数(data)
    //      其中参数是variant_object形式,想获得其中一个参数的值可以使用类似方法
    //          asset amount = data["amount"].as<asset>();
    //      按目前的业务需求,最终分析好action,来确定是否需要调用 
    //          add_amount (string account, double amount);
    //      其中account是用户的账户名称 amount可正可负表示增加金额还是减少金额
    //      比如下面是对eosio.token的transfer方法分析
    /*
    if (account == "eosio.token" && name == "transfer") {
        printf ("enter here\n");
        auto data = trace_variant.get_object()["act"].get_object()["data"].get_object();
        auto from = data["from"].as<string>();
        auto to = data["to"].as<string>();
        auto quantity = data["quantity"].as<asset>(); 
        if (quantity.get_symbol() != symbol(4, "BOS")) return;
        double amount = quantity.to_real();
        dlog ("eosio::transfer [from=${from}] [to=${to}] [quantity=${quantity}]", ("from", from)("to", to)("quantity", quantity));
        if (from != "eosio") {
            add_amount(from, amount * -1);
        }
        if (to != "eosio") {
            add_amount(to, amount * 1);
        }
    }
    */
}
    
void mysql_plugin::add_amount (string account, double amount) {
    char sql[256] = {0};
    sprintf(sql,
            "insert into account (account, amount) values('%s', %lf) "
            "on duplicate key update "
            "amount = amount + %lf",
            account.c_str(), amount, amount);
    dlog ("sql : ${sql}", ("sql", sql));
    if (mysql_query(mysql_connection, sql)) {
        elog ("execute query '${sql}' failed [error=${error}]", ("sql", sql)("error", mysql_error(mysql_connection)));
        return;
    }
    auto mysql_result = mysql_use_result(mysql_connection);
    if (mysql_result == NULL) {
        return;
    }
    mysql_free_result(mysql_result);
}

}
