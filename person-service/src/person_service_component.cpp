#include "person_service_component.hpp"

#include <userver/storages/postgres/component.hpp>

namespace person_service {

PersonServiceComponent::PersonServiceComponent(const userver::components::ComponentConfig& config,
                                               const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      repository_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      service_(repository_) {
    repository_.EnsureSchema();
}

PersonService& PersonServiceComponent::GetService() {
    return service_;
}

} // namespace person_service
