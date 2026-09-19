#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "domain/person.hpp"

namespace person_service {

// Storage port; implemented on Postgres in storage/, mocked in unit tests.
class PersonRepository {
  public:
    PersonRepository() = default;
    PersonRepository(const PersonRepository&) = delete;
    PersonRepository(PersonRepository&&) = delete;
    PersonRepository& operator=(const PersonRepository&) = delete;
    PersonRepository& operator=(PersonRepository&&) = delete;
    virtual ~PersonRepository() = default;

    virtual std::vector<Person> FindAll() const = 0;
    virtual std::optional<Person> FindById(std::int32_t id) const = 0;
    // Ignores person.id; returns the id assigned by the storage.
    virtual std::int32_t Insert(const Person& person) = 0;
    // Returns false if there is no person with person.id.
    virtual bool Update(const Person& person) = 0;
    // Returns false if there is no person with this id.
    virtual bool Delete(std::int32_t id) = 0;
};

} // namespace person_service
