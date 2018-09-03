#include <string>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

/*
   Import the PathMatcher method declarations AND definitions
   since we're mocking the injected dependencies
*/
#include "ParsingException.hpp"
#include "PathMatcher.cpp"
#include "PathMatcher.hpp"

namespace {
using ::testing::Return;

class MockUserInfo {
 public:
  MOCK_CONST_METHOD0(getUsername, std::string());
  MOCK_CONST_METHOD0(getUid, int());
  MOCK_CONST_METHOD0(getHomeDirectory, std::string());
};

class MockGroupInfo {
 public:
  MOCK_CONST_METHOD0(getGroupname, std::string());
  MOCK_CONST_METHOD0(getGid, int());
};

class PathMatcherTest : public ::testing::Test {
 protected:
  PathMatcherTest() : path_matcher(mock_userinfo, mock_groupinfo){};
  MockUserInfo mock_userinfo;
  MockGroupInfo mock_groupinfo;
  suPHP::PathMatcher<MockUserInfo, MockGroupInfo> path_matcher;
};

TEST_F(PathMatcherTest, StaticPathsEndingSlash) {
  ASSERT_TRUE(path_matcher.matches("/", "/home/foo"));
  ASSERT_TRUE(path_matcher.matches("/home/", "/home/foo"));
  ASSERT_TRUE(path_matcher.matches("/home/", "/home/bar"));
}

TEST_F(PathMatcherTest, StaticPathsImplicitPatternSlash) {
  // A pattern that does not end in '/' can match either a file
  // or directory of the same name.
  ASSERT_TRUE(path_matcher.matches("", "/home/foo"));
  ASSERT_TRUE(path_matcher.matches("/xyzzy", "/xyzzy"));
  ASSERT_TRUE(path_matcher.matches("/xyzzy", "/xyzzy/"));
  ASSERT_TRUE(path_matcher.matches("/xyzzy", "/xyzzy/abcde"));
  ASSERT_FALSE(path_matcher.matches("/xyzzy", "/xyzzyabcde"));
  ASSERT_FALSE(path_matcher.matches("/xyzzy", "/abcde"));
}

TEST_F(PathMatcherTest, StaticPathsImplicitPathSlash) {
  // A path that does not end in '/' is treated as a filename
  ASSERT_FALSE(path_matcher.matches("/home/", "/home"));
  ASSERT_FALSE(path_matcher.matches("/./", "/."));
}

TEST_F(PathMatcherTest, EscapedPatternEndingSlash) {
  ASSERT_TRUE(path_matcher.matches("/home/\\*/", "/home/*/"));
  ASSERT_TRUE(path_matcher.matches("/home/\\*/", "/home/*/x"));
  ASSERT_FALSE(path_matcher.matches("/home/\\*/", "/home/*"));
}

TEST_F(PathMatcherTest, EscapedPatternImplicitSlash) {
  ASSERT_TRUE(path_matcher.matches("/home/\\*", "/home/*/"));
  ASSERT_TRUE(path_matcher.matches("/home/\\*", "/home/*/x"));
  ASSERT_TRUE(path_matcher.matches("/home/\\*", "/home/*/x"));
  ASSERT_FALSE(path_matcher.matches("/home/\\*", "/home/*x"));
  ASSERT_FALSE(path_matcher.matches("/home/\\*", "/home/X"));
  ASSERT_TRUE(path_matcher.matches("/\\\\", "/\\"));
  ASSERT_TRUE(path_matcher.matches("/\\\\", "/\\/"));
  ASSERT_TRUE(path_matcher.matches("/abc\\\\def\\$ghi", "/abc\\def$ghi/"));
}

TEST_F(PathMatcherTest, FaultyEscapes) {
  // A faulty escape sequence is handled as a plain backslash
  ASSERT_FALSE(path_matcher.matches("/home/\\a", "/home/a/bc"));
  ASSERT_TRUE(path_matcher.matches("/home/\\a", "/home/\\a/bc"));
  ASSERT_FALSE(path_matcher.matches("/home/\\b", "/home/"));
  ASSERT_TRUE(path_matcher.matches("/home/\\", "/home/\\/"));

  // No support for printf style escape sequences
  ASSERT_FALSE(path_matcher.matches("/home/\\ndef/", "/home/\ndef/"));
}

TEST_F(PathMatcherTest, InterpolatedVar) {
  EXPECT_CALL(mock_userinfo, getUsername())
      .Times(4)
      .WillRepeatedly(Return("foo"));
  ASSERT_TRUE(path_matcher.matches("/home/${USERNAME}", "/home/foo/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/${USERNAME}/", "/home/foo/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/\\$${USERNAME}", "/home/$foo/"));
  ASSERT_FALSE(path_matcher.matches("/home/${USERNAME}/", "/home/bar/baz"));
  EXPECT_THROW(path_matcher.matches("/home/$USER{NAME}", "/home/$USER{NAME}/"),
               suPHP::ParsingException);
}

TEST_F(PathMatcherTest, DoubleInterpolation) {
  EXPECT_CALL(mock_userinfo, getUsername())
      .Times(2)
      .WillRepeatedly(Return("${UID}"));
  ASSERT_TRUE(path_matcher.matches("/home/${USERNAME}", "/home/${UID}/"));
  ASSERT_TRUE(path_matcher.matches("/home/${USERNAME}/", "/home/${UID}/"));
}

TEST_F(PathMatcherTest, WildCards) {
  EXPECT_CALL(mock_userinfo, getUsername())
      .Times(3)
      .WillRepeatedly(Return("foo"));
  ASSERT_TRUE(path_matcher.matches("/*/${USERNAME}", "/home/foo/"));
  ASSERT_TRUE(path_matcher.matches("/home/${USERNAME}/*", "/home/foo/bar/"));
  ASSERT_TRUE(path_matcher.matches("/home/${USERNAME}/*", "/home/foo/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/f*o/*", "/home/foo/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/*o/*", "/home/foo/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/f*/*", "/home/foo/bar"));
  ASSERT_FALSE(path_matcher.matches("/home/f*/z*", "/home/foo/bar"));
  ASSERT_FALSE(path_matcher.matches("/home/*z*/*", "/home/foo/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/*\\*", "/home/foo*/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/*\\*/", "/home/foo*/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/\\**", "/home/*foo/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/\\**/", "/home/*foo/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/\\**", "/home/*/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/\\**/", "/home/*/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/*\\*", "/home/*/bar"));
  ASSERT_TRUE(path_matcher.matches("/home/*\\*/", "/home/*/bar"));
  // PathMatcher can't handle simple path traversal
  //ASSERT_FALSE(path_matcher.matches("/home/*/", "/home/./"));
}
}
