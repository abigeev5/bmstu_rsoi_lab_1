#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "domain/person.hpp"
#include "domain/person_repository.hpp"
#include "domain/person_service.hpp"

namespace person_service {
namespace {

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Return;
using ::testing::StrictMock;

class MockPersonRepository : public PersonRepository {
  public:
    MOCK_METHOD(std::vector<Person>, FindAll, (), (const, override));
    MOCK_METHOD(std::optional<Person>, FindById, (std::int32_t id), (const, override));
    MOCK_METHOD(std::int32_t, Insert, (const Person& person), (override));
    MOCK_METHOD(bool, Update, (const Person& person), (override));
    MOCK_METHOD(bool, Delete, (std::int32_t id), (override));
};

PersonRequest MakePatch(std::optional<std::string> name,
                        std::optional<std::string> address = std::nullopt) {
    PersonRequest patch;
    patch.name = std::move(name);
    patch.address = std::move(address);
    return patch;
}

Person MakePerson(std::int32_t id) {
    return Person{.id = id, .name = "Ann", .age = 31, .address = "Street", .work = "Job"};
}

class PersonServiceTest : public ::testing::Test {
  protected:
    StrictMock<MockPersonRepository> repository_;
    PersonService service_{repository_};
};

TEST_F(PersonServiceTest, CreateReturnsIdAssignedByRepository) {
    const auto person = MakePerson(0);
    EXPECT_CALL(repository_, Insert(person)).WillOnce(Return(42));

    EXPECT_EQ(service_.Create(person), 42);
}

TEST_F(PersonServiceTest, GetReturnsPersonOrNullopt) {
    EXPECT_CALL(repository_, FindById(1)).WillOnce(Return(MakePerson(1)));
    EXPECT_CALL(repository_, FindById(2)).WillOnce(Return(std::nullopt));

    EXPECT_EQ(service_.Get(1), MakePerson(1));
    EXPECT_EQ(service_.Get(2), std::nullopt);
}

TEST_F(PersonServiceTest, GetAllReturnsEveryPerson) {
    EXPECT_CALL(repository_, FindAll())
        .WillOnce(Return(std::vector<Person>{MakePerson(1), MakePerson(2)}));

    EXPECT_THAT(service_.GetAll(), ElementsAre(MakePerson(1), MakePerson(2)));
}

TEST_F(PersonServiceTest, PatchChangesOnlyProvidedFields) {
    const auto patch = MakePatch("Bob", "Avenue");
    auto expected = MakePerson(1);
    expected.name = "Bob";
    expected.address = "Avenue";

    EXPECT_CALL(repository_, FindById(1)).WillOnce(Return(MakePerson(1)));
    EXPECT_CALL(repository_, Update(expected)).WillOnce(Return(true));

    EXPECT_EQ(service_.Patch(1, patch), expected);
}

TEST_F(PersonServiceTest, PatchOfMissingPersonReturnsNulloptWithoutUpdate) {
    EXPECT_CALL(repository_, FindById(7)).WillOnce(Return(std::nullopt));

    EXPECT_EQ(service_.Patch(7, MakePatch("Bob")), std::nullopt);
}

TEST_F(PersonServiceTest, PatchOfConcurrentlyDeletedPersonReturnsNullopt) {
    EXPECT_CALL(repository_, FindById(1)).WillOnce(Return(MakePerson(1)));
    EXPECT_CALL(repository_, Update(_)).WillOnce(Return(false));

    EXPECT_EQ(service_.Patch(1, MakePatch("Bob")), std::nullopt);
}

TEST_F(PersonServiceTest, DeleteReportsWhetherPersonExisted) {
    EXPECT_CALL(repository_, Delete(1)).WillOnce(Return(true));
    EXPECT_CALL(repository_, Delete(7)).WillOnce(Return(false));

    EXPECT_TRUE(service_.Delete(1));
    EXPECT_FALSE(service_.Delete(7));
}

} // namespace
} // namespace person_service
