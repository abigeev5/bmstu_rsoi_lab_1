add_executable(person-service-tests
    person_service_test.cpp)
target_link_libraries(person-service-tests PRIVATE person_domain userver::utest)
rsoi_target_defaults(person-service-tests)

add_google_tests(person-service-tests)
