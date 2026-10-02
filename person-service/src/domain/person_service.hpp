#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "domain/person.hpp"
#include "domain/person_repository.hpp"

namespace person_service {

void ApplyPatch(Person& person, const PersonRequest& patch);

class PersonService {
  public:
    explicit PersonService(PersonRepository& repository);

    std::vector<Person> GetAll() const;
    std::optional<Person> Get(std::int32_t id) const;
    std::int32_t Create(const Person& person);
    std::optional<Person> Patch(std::int32_t id, const PersonRequest& patch);
    bool Delete(std::int32_t id);

  private:
    PersonRepository& repository_;
};

} // namespace person_service
