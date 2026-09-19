#pragma once

#include <cstdint>
#include <expected>
#include <map>
#include <string>
#include <string_view>

#include <userver/formats/json/value.hpp>
#include <userver/formats/serialize/to.hpp>

#include "domain/person.hpp"

namespace person_service {

// Found by ADL: formats::json::ValueBuilder{person}.
userver::formats::json::Value
Serialize(const Person& person,
          userver::formats::serialize::To<userver::formats::json::Value> /*to*/);

} // namespace person_service

namespace person_service::api {

// field -> error text, rendered as ValidationErrorResponse.errors
using ValidationErrors = std::map<std::string, std::string>;

enum class NameRule : std::uint8_t { kRequired, kOptional };

std::expected<PersonRequest, ValidationErrors> ParsePersonRequest(std::string_view body,
                                                                  NameRule name_rule);

std::string MakeErrorBody(std::string_view message);
std::string MakeValidationErrorBody(const ValidationErrors& errors);

} // namespace person_service::api
