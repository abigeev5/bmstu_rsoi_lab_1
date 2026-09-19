#include "api/person_json.hpp"

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace person_service {

userver::formats::json::Value
Serialize(const Person& person,
          userver::formats::serialize::To<userver::formats::json::Value> /*to*/) {
    userver::formats::json::ValueBuilder builder{userver::formats::common::Type::kObject};
    builder["id"] = person.id;
    builder["name"] = person.name;
    if (person.age) {
        builder["age"] = *person.age;
    }
    if (person.address) {
        builder["address"] = *person.address;
    }
    if (person.work) {
        builder["work"] = *person.work;
    }
    return builder.ExtractValue();
}

} // namespace person_service

namespace person_service::api {

namespace {

// Absent and null fields are treated the same: "not provided".
bool IsProvided(const userver::formats::json::Value& value) {
    return !value.IsMissing() && !value.IsNull();
}

std::optional<std::string> ParseString(const userver::formats::json::Value& json,
                                       const std::string& field, ValidationErrors& errors) {
    const auto value = json[field];
    if (!IsProvided(value)) {
        return std::nullopt;
    }
    if (!value.IsString()) {
        errors[field] = "must be a string";
        return std::nullopt;
    }
    return value.As<std::string>();
}

} // namespace

std::expected<PersonRequest, ValidationErrors> ParsePersonRequest(std::string_view body,
                                                                  NameRule name_rule) {
    userver::formats::json::Value json;
    try {
        json = userver::formats::json::FromString(body);
    } catch (const userver::formats::json::Exception&) {
        return std::unexpected(ValidationErrors{{"body", "must be valid JSON"}});
    }
    if (!json.IsObject()) {
        return std::unexpected(ValidationErrors{{"body", "must be a JSON object"}});
    }

    ValidationErrors errors;
    PersonRequest request;

    request.name = ParseString(json, "name", errors);
    if (!errors.contains("name")) {
        if (!request.name && name_rule == NameRule::kRequired) {
            errors["name"] = "is required";
        } else if (request.name && request.name->empty()) {
            errors["name"] = "must not be empty";
        }
    }

    const auto age = json["age"];
    if (IsProvided(age)) {
        if (age.IsInt()) {
            request.age = age.As<std::int32_t>();
        } else {
            errors["age"] = "must be a 32-bit integer";
        }
    }

    request.address = ParseString(json, "address", errors);
    request.work = ParseString(json, "work", errors);

    if (!errors.empty()) {
        return std::unexpected(std::move(errors));
    }
    return request;
}

std::string MakeErrorBody(std::string_view message) {
    userver::formats::json::ValueBuilder builder;
    builder["message"] = std::string{message};
    return userver::formats::json::ToString(builder.ExtractValue());
}

std::string MakeValidationErrorBody(const ValidationErrors& errors) {
    userver::formats::json::ValueBuilder builder;
    builder["message"] = "Invalid data";
    builder["errors"] =
        userver::formats::json::ValueBuilder{userver::formats::common::Type::kObject};
    for (const auto& [field, text] : errors) {
        builder["errors"][field] = text;
    }
    return userver::formats::json::ToString(builder.ExtractValue());
}

} // namespace person_service::api
