#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "domain/person.hpp"
#include "domain/person_repository.hpp"

namespace person_service {

// Overwrites only the fields present in the patch.
void ApplyPatch(Person& person, const PersonRequest& patch);

// Input is expected to be validated by the API layer (e.g. name is present for Create).
class PersonService {
  public:
    explicit PersonService(PersonRepository& repository);

    std::vector<Person> GetAll() const;
    std::optional<Person> Get(std::int32_t id) const;
    std::int32_t Create(const Person& person);
    // Returns the updated person, or nullopt if it does not exist.
    std::optional<Person> Patch(std::int32_t id, const PersonRequest& patch);
    // Returns false if the person does not exist.
    bool Delete(std::int32_t id);

  private:
    PersonRepository& repository_;
};

} // namespace person_service
