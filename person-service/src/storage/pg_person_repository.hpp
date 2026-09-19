#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <userver/storages/postgres/postgres_fwd.hpp>

#include "domain/person.hpp"
#include "domain/person_repository.hpp"

namespace person_service::storage {

class PgPersonRepository final : public PersonRepository {
  public:
    explicit PgPersonRepository(userver::storages::postgres::ClusterPtr cluster);

    // Creates the table if it does not exist yet (the service has no separate migrations).
    void EnsureSchema();

    std::vector<Person> FindAll() const override;
    std::optional<Person> FindById(std::int32_t id) const override;
    std::int32_t Insert(const Person& person) override;
    bool Update(const Person& person) override;
    bool Delete(std::int32_t id) override;

  private:
    userver::storages::postgres::ClusterPtr cluster_;
};

} // namespace person_service::storage
