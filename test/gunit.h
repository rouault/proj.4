#include "catch.hpp"

#define STR(x) #x
#define CONCAT(x,y) STR(x ## _ ## y)

#define TEST(x,y) TEST_CASE(CONCAT(x,y))
#define ASSERT_EQ(x,y) CHECK( (x) == (y) )
#define EXPECT_DOUBLE_EQ(x,y) CHECK( (x) == (y) )
#define EXPECT_TRUE(x) CHECK(x)
#define EXPECT_NEAR(x,y,tolerance) CHECK( (x) == Approx(y).margin(tolerance) )