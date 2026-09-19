#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace person_service {

struct Person {
    std::int32_t id{};
    std::string name;
    std::optional<std::int32_t> age;
    std::optional<std::string> address;
    std::optional<std::string> work;

    bool operator==(const Person&) const = default;
};

// Body of POST (name is required) and PATCH (every field is optional, absent fields stay
// unchanged).
struct PersonRequest {
    std::optional<std::string> name;
    std::optional<std::int32_t> age;
    std::optional<std::string> address;
    std::optional<std::string> work;

    bool operator==(const PersonRequest&) const = default;
};

} // namespace person_service
