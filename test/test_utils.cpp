#include "gtest/gtest.h"
#include <uriparser/Uri.h>
#include "test_utils.h"


TEST(UtilsTest, ParseUrl)
{
    UriUriA uri;
    auto uriResult = uriParseSingleUriA(&uri, "http://example.com/test?abc=123", nullptr);
    ASSERT_EQ(uriResult,URI_SUCCESS);

    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.scheme),"http");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.hostText),"example.com");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.pathHead->text),"test");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.query),"abc=123");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.portText),"");
    uriFreeUriMembersA(&uri);
}

TEST(UtilsTest, ParseUrlWithPort)
{
    UriUriA uri;
    auto uriResult = uriParseSingleUriA(&uri, "http://example.com:8080/test?abc=123", nullptr);
    ASSERT_EQ(uriResult,URI_SUCCESS);

    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.scheme),"http");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.hostText),"example.com");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.pathHead->text),"test");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.query),"abc=123");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.portText),"8080");
    uriFreeUriMembersA(&uri);
}

TEST(UtilsTest, ParseUrlNoScheme)
{
    UriUriA uri;
    auto uriResult = uriParseSingleUriA(&uri, "//example.com/test?abc=123", nullptr);
    ASSERT_EQ(uriResult,URI_SUCCESS);

    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.scheme),"");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.hostText),"example.com");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.pathHead->text),"test");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.query),"abc=123");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.portText),"");
    uriFreeUriMembersA(&uri);
}

TEST(UtilsTest, ParseUrlMultiplePathSegments)
{
    UriUriA uri;
    auto uriResult = uriParseSingleUriA(&uri, "http://example.com/test/test1/test2?abc=123", nullptr);
    ASSERT_EQ(uriResult,URI_SUCCESS);

    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.scheme),"http");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.hostText),"example.com");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.query),"abc=123");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.portText),"");
    auto path = uri.pathHead;
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(path->text),"test");
    path = path->next;
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(path->text),"test1");
    path = path->next;
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(path->text),"test2");
    EXPECT_EQ(path,uri.pathTail);

    uriFreeUriMembersA(&uri);
}

