#pragma once

#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include "domain/person_service.hpp"
#include "storage/pg_person_repository.hpp"

namespace person_service {

// Wires the Postgres repository into PersonService and shares it with the handlers.
class PersonServiceComponent final : public userver::components::ComponentBase {
  public:
    static constexpr std::string_view kName = "person-service";

    PersonServiceComponent(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context);

    PersonService& GetService();

  private:
    storage::PgPersonRepository repository_;
    PersonService service_;
};

} // namespace person_service
