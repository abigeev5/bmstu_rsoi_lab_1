#include "domain/person_service.hpp"

namespace person_service {

void ApplyPatch(Person& person, const PersonRequest& patch) {
    if (patch.name) {
        person.name = *patch.name;
    }
    if (patch.age) {
        person.age = patch.age;
    }
    if (patch.address) {
        person.address = patch.address;
    }
    if (patch.work) {
        person.work = patch.work;
    }
}

PersonService::PersonService(PersonRepository& repository) : repository_(repository) {}

std::vector<Person> PersonService::GetAll() const {
    return repository_.FindAll();
}

std::optional<Person> PersonService::Get(std::int32_t id) const {
    return repository_.FindById(id);
}

std::int32_t PersonService::Create(const Person& person) {
    return repository_.Insert(person);
}

std::optional<Person> PersonService::Patch(std::int32_t id, const PersonRequest& patch) {
    auto person = repository_.FindById(id);
    if (!person) {
        return std::nullopt;
    }
    ApplyPatch(*person, patch);
    if (!repository_.Update(*person)) {
        return std::nullopt; // deleted between FindById and Update
    }
    return person;
}

bool PersonService::Delete(std::int32_t id) {
    return repository_.Delete(id);
}

} // namespace person_service
