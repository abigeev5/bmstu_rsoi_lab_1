#pragma once

#include <string>
#include <string_view>

#include <userver/server/handlers/http_handler_base.hpp>

namespace person_service::api {

// /api/v1/persons: GET (list), POST (create)
class PersonsHandler final : public userver::server::handlers::HttpHandlerBase {
  public:
    static constexpr std::string_view kName = "handler-persons";

    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequest(userver::server::http::HttpRequest& request,
                              userver::server::request::RequestContext& context) const override;
};

// /api/v1/persons/{id}: GET, PATCH, DELETE
class PersonHandler final : public userver::server::handlers::HttpHandlerBase {
  public:
    static constexpr std::string_view kName = "handler-person";

    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequest(userver::server::http::HttpRequest& request,
                              userver::server::request::RequestContext& context) const override;
};

} // namespace person_service::api
