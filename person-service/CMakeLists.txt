add_library(person_domain STATIC
    src/domain/person_service.cpp)
target_include_directories(person_domain PUBLIC src)
target_link_libraries(person_domain PUBLIC userver::core)
rsoi_target_defaults(person_domain)

add_executable(person-service
    src/main.cpp
    src/api/person_handlers.cpp
    src/api/person_json.cpp
    src/storage/pg_person_repository.cpp)
target_link_libraries(person-service PRIVATE person_domain userver::postgresql)
rsoi_target_defaults(person-service)

add_subdirectory(tests)
