#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/utils/daemon_run.hpp>

#include "api/person_handlers.hpp"

int main(int argc, char* argv[]) {
    const auto component_list = userver::components::MinimalServerComponentList()
                                    .Append<userver::server::handlers::Ping>()
                                    .Append<person_service::api::PersonsHandler>()
                                    .Append<person_service::api::PersonHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
