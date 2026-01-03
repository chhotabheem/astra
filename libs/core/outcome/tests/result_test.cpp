#include "Result.h"

#include <gtest/gtest.h>
#include <string>

using namespace astra::outcome;

struct TestError {
  int code;
  std::string message;
};

TEST(ResultTest, OkValue) {
  auto result = Result<int, TestError>::Ok(42);

  EXPECT_TRUE(result.is_ok());
  EXPECT_FALSE(result.is_err());
  EXPECT_EQ(result.value(), 42);
}

TEST(ResultTest, ErrValue) {
  auto result = Result<int, TestError>::Err(TestError{404, "Not found"});

  EXPECT_FALSE(result.is_ok());
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.error().code, 404);
  EXPECT_EQ(result.error().message, "Not found");
}

TEST(ResultTest, ValueOr) {
  auto ok_result = Result<int, TestError>::Ok(42);
  auto err_result = Result<int, TestError>::Err(TestError{500, "Error"});

  EXPECT_EQ(ok_result.value_or(0), 42);
  EXPECT_EQ(err_result.value_or(0), 0);
}

TEST(ResultTest, BoolConversion) {
  auto ok_result = Result<int, TestError>::Ok(42);
  auto err_result = Result<int, TestError>::Err(TestError{500, "Error"});

  EXPECT_TRUE(ok_result);
  EXPECT_FALSE(err_result);
}

TEST(ResultTest, MoveSemantics) {
  auto result = Result<std::string, TestError>::Ok(std::string("hello"));

  std::string value = std::move(result).value();
  EXPECT_EQ(value, "hello");
}
