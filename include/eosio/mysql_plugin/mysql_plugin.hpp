#pragma once

#include <mysql.h>

#include <fc/io/json.hpp>
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
namespace eosio {

using namespace eosio::chain;

using namespace appbase;

class mysql_plugin : public appbase::plugin<mysql_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin))

    void set_program_options(options_description&, options_description& cfg) override;
    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

private:

    bool enable;

    boost::signals2::connection on_accepted_block_connection;
    boost::signals2::connection on_irreversible_block_connection;
    boost::signals2::connection on_applied_transaction_connection;
    boost::signals2::connection on_accepted_transaction_connection;

    MYSQL* mysql_connection;

private:
    void analyse_action (action_trace& trace);
    void add_amount (string account, double amount);
};

}
