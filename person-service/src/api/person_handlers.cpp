#include "api/person_handlers.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/utils/from_string.hpp>

#include "api/person_json.hpp"
#include "domain/person.hpp"

namespace person_service::api {

namespace {

using userver::server::http::HttpMethod;
using userver::server::http::HttpRequest;
using userver::server::http::HttpStatus;

// TODO(lab1): replace the stub with PersonService backed by Postgres.
constexpr std::int32_t kStubPersonId = 1;

Person StubPerson() {
    return Person{
        .id = kStubPersonId,
        .name = "Stub",
        .age = 30,
        .address = "Stub street",
        .work = "Stub inc",
    };
}

std::optional<Person> StubFind(std::int32_t id) {
    if (id != kStubPersonId) {
        return std::nullopt;
    }
    return StubPerson();
}

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

std::string Respond(const HttpRequest& request, HttpStatus status, std::string body) {
    request.SetResponseStatus(status);
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
    return body;
}

template <typename T>
std::string RespondJson(const HttpRequest& request, HttpStatus status, const T& value) {
    return Respond(request, status,
                   userver::formats::json::ToString(
                       userver::formats::json::ValueBuilder{value}.ExtractValue()));
}

std::string RespondNotFound(const HttpRequest& request, std::int32_t id) {
    return Respond(request, HttpStatus::kNotFound,
                   MakeErrorBody("Person with id " + std::to_string(id) + " not found"));
}

std::string RespondValidationError(const HttpRequest& request, const ValidationErrors& errors) {
    return Respond(request, HttpStatus::kBadRequest, MakeValidationErrorBody(errors));
}

std::optional<std::int32_t> ParseId(const std::string& raw) {
    try {
        return userver::utils::FromString<std::int32_t>(raw);
    } catch (const userver::utils::FromStringException&) {
        return std::nullopt;
    }
}

} // namespace

std::string
PersonsHandler::HandleRequest(HttpRequest& request,
                              userver::server::request::RequestContext& /*context*/) const {
    if (request.GetMethod() == HttpMethod::kGet) {
        return RespondJson(request, HttpStatus::kOk, std::vector<Person>{StubPerson()});
    }

    // POST
    const auto person_request = ParsePersonRequest(request.RequestBody(), NameRule::kRequired);
    if (!person_request) {
        return RespondValidationError(request, person_request.error());
    }
    request.SetResponseStatus(HttpStatus::kCreated);
    request.GetHttpResponse().SetHeader(userver::http::headers::kLocation,
                                        "/api/v1/persons/" + std::to_string(kStubPersonId));
    return {};
}

std::string
PersonHandler::HandleRequest(HttpRequest& request,
                             userver::server::request::RequestContext& /*context*/) const {
    const auto id = ParseId(request.GetPathArg("id"));
    if (!id) {
        return RespondValidationError(request, {{"id", "must be a 32-bit integer"}});
    }

    auto person = StubFind(*id);
    if (!person) {
        return RespondNotFound(request, *id);
    }

    switch (request.GetMethod()) {
    case HttpMethod::kGet:
        return RespondJson(request, HttpStatus::kOk, *person);
    case HttpMethod::kPatch: {
        const auto patch = ParsePersonRequest(request.RequestBody(), NameRule::kOptional);
        if (!patch) {
            return RespondValidationError(request, patch.error());
        }
        ApplyPatch(*person, *patch);
        return RespondJson(request, HttpStatus::kOk, *person);
    }
    default: // DELETE; other methods are rejected by the handler config
        request.SetResponseStatus(HttpStatus::kNoContent);
        return {};
    }
}

} // namespace person_service::api
