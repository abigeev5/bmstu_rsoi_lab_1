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
#include "domain/person_service.hpp"
#include "person_service_component.hpp"

namespace person_service::api {

namespace {

using userver::server::http::HttpMethod;
using userver::server::http::HttpRequest;
using userver::server::http::HttpStatus;

// POST body is validated with NameRule::kRequired, so name is present.
Person ToNewPerson(const PersonRequest& request) {
    return Person{
        .id = 0,
        .name = request.name.value_or(""),
        .age = request.age,
        .address = request.address,
        .work = request.work,
    };
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

PersonsHandler::PersonsHandler(const userver::components::ComponentConfig& config,
                               const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      service_(context.FindComponent<PersonServiceComponent>().GetService()) {}

std::string
PersonsHandler::HandleRequest(HttpRequest& request,
                              userver::server::request::RequestContext& /*context*/) const {
    if (request.GetMethod() == HttpMethod::kGet) {
        return RespondJson(request, HttpStatus::kOk, service_.GetAll());
    }

    // POST
    const auto person_request = ParsePersonRequest(request.RequestBody(), NameRule::kRequired);
    if (!person_request) {
        return RespondValidationError(request, person_request.error());
    }
    const auto id = service_.Create(ToNewPerson(*person_request));
    request.SetResponseStatus(HttpStatus::kCreated);
    request.GetHttpResponse().SetHeader(userver::http::headers::kLocation,
                                        "/api/v1/persons/" + std::to_string(id));
    return {};
}

PersonHandler::PersonHandler(const userver::components::ComponentConfig& config,
                             const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      service_(context.FindComponent<PersonServiceComponent>().GetService()) {}

std::string
PersonHandler::HandleRequest(HttpRequest& request,
                             userver::server::request::RequestContext& /*context*/) const {
    const auto id = ParseId(request.GetPathArg("id"));
    if (!id) {
        return RespondValidationError(request, {{"id", "must be a 32-bit integer"}});
    }

    switch (request.GetMethod()) {
    case HttpMethod::kGet: {
        const auto person = service_.Get(*id);
        if (!person) {
            return RespondNotFound(request, *id);
        }
        return RespondJson(request, HttpStatus::kOk, *person);
    }
    case HttpMethod::kPatch: {
        const auto patch = ParsePersonRequest(request.RequestBody(), NameRule::kOptional);
        if (!patch) {
            return RespondValidationError(request, patch.error());
        }
        const auto person = service_.Patch(*id, *patch);
        if (!person) {
            return RespondNotFound(request, *id);
        }
        return RespondJson(request, HttpStatus::kOk, *person);
    }
    default: // DELETE; other methods are rejected by the handler config
        if (!service_.Delete(*id)) {
            return RespondNotFound(request, *id);
        }
        request.SetResponseStatus(HttpStatus::kNoContent);
        return {};
    }
}

} // namespace person_service::api
