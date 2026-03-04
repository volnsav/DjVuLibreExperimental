#include <gtest/gtest.h>
#include "GString.h"

TEST(GUTF8StringTest, Constructor)
{
    GUTF8String s;
    EXPECT_EQ(0, s.length());

    GUTF8String s2("hello");
    EXPECT_EQ(5, s2.length());

    GUTF8String s3(s2);
    EXPECT_EQ(5, s3.length());
    EXPECT_STREQ(s2, s3);
}

TEST(GUTF8StringTest, Comparison)
{
    GUTF8String s1("hello");
    GUTF8String s2("hello");
    GUTF8String s3("world");

    EXPECT_EQ(s1, s2);
    EXPECT_NE(s1, s3);
}

TEST(GUTF8StringTest, Concatenation)
{
    GUTF8String s1("hello");
    GUTF8String s2(" world");
    GUTF8String s3 = s1 + s2;
    EXPECT_STREQ("hello world", s3);
}

TEST(GUTF8StringTest, Substr)
{
    GUTF8String s("hello world");
    GUTF8String sub = s.substr(6, 5);
    EXPECT_STREQ("world", sub);
}

TEST(GUTF8StringTest, CaseConversion)
{
    GUTF8String s("HeLLo");
    GUTF8String upper = s.upcase();
    GUTF8String lower = s.downcase();
    EXPECT_STREQ("HELLO", upper);
    EXPECT_STREQ("hello", lower);
}

TEST(GUTF8StringTest, ToInt)
{
    GUTF8String s("12345");
    EXPECT_EQ(12345, s.toInt());

    GUTF8String s2("-543");
    EXPECT_EQ(-543, s2.toInt());
    
    GUTF8String s3("not a number");
    EXPECT_EQ(0, s3.toInt());
}

TEST(GUTF8StringTest, Basic)
{
    GUTF8String text("abc");
    EXPECT_EQ(text.length(), 3);
    EXPECT_EQ(text.contains("b"), 1);
    EXPECT_EQ(text.rcontains("b"), 1);
    EXPECT_TRUE(text == "abc");
}
