#include "storage/pg_person_repository.hpp"

#include <utility>

#include <userver/storages/postgres/cluster.hpp>

#include <person_service/sql_queries.hpp>

namespace person_service::storage {

namespace {

namespace pg = userver::storages::postgres;

constexpr auto kMaster = pg::ClusterHostType::kMaster;

} // namespace

PgPersonRepository::PgPersonRepository(pg::ClusterPtr cluster) : cluster_(std::move(cluster)) {}

void PgPersonRepository::EnsureSchema() {
    cluster_->Execute(kMaster, sql::kCreatePersonsTable);
}

// Reads go to the master too: a replica could lag behind a just-created person.
std::vector<Person> PgPersonRepository::FindAll() const {
    return cluster_->Execute(kMaster, sql::kSelectPersons)
        .AsContainer<std::vector<Person>>(pg::kRowTag);
}

std::optional<Person> PgPersonRepository::FindById(std::int32_t id) const {
    return cluster_->Execute(kMaster, sql::kSelectPerson, id)
        .AsOptionalSingleRow<Person>(pg::kRowTag);
}

std::int32_t PgPersonRepository::Insert(const Person& person) {
    return cluster_
        ->Execute(kMaster, sql::kInsertPerson, person.name, person.age, person.address, person.work)
        .AsSingleRow<std::int32_t>();
}

bool PgPersonRepository::Update(const Person& person) {
    return cluster_
               ->Execute(kMaster, sql::kUpdatePerson, person.id, person.name, person.age,
                         person.address, person.work)
               .RowsAffected() > 0;
}

bool PgPersonRepository::Delete(std::int32_t id) {
    return cluster_->Execute(kMaster, sql::kDeletePerson, id).RowsAffected() > 0;
}

} // namespace person_service::storage
