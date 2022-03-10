#include "gtest/gtest.h"
#include <filesystem>
#include "htmlparser.h"
#include <uriparser/Uri.h>
#include "test_utils.h"

TEST(HtmlParserTest, ParseStreamableHtml)
{
    std::filesystem::path streamable_html(RDTEST_DATA_FOLDER);
    streamable_html /= "streamable.html";
    HtmlParser parser(streamable_html);
    auto url = parser.getMediaLink("streamable.com");
    UriUriA uri;
    auto uriResult = uriParseSingleUriA(&uri, url.url.c_str(), nullptr);
    ASSERT_EQ(uriResult,URI_SUCCESS);

    EXPECT_FALSE(url.url.empty());
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.hostText),"cdn-cf-east.streamable.com");
    auto path = uri.pathHead;
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(path->text),"video");
    path = path->next;
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(path->text),"mp4");
    path = path->next;
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(path->text),"sh7m2g.mp4");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.query),"Expires=1608061320&Signature=Hr~3Vu0BrnNckKtaY8R-rqlTVwB00WKF1s0F6RKUiXu1Qx9dCJ5wvaM83d1IFuPDkSvGawrwUQEY"
              "s0-m7XGqL8GzUMCt-OXAr2C7gzoJICIii1BQX3vNBGS2-mDMnFCm-G26bZg1qE6-ToKU~nyY1o4JR85IETAi1UHvNVJADGFoW-G6z1A7nfxmm3S8D4vPMWr9mK5hkOoWIUa9pnJiTCF1ZXfVxLXhDKSqZXWVcJJKJxSPOJiHAM8Ph29Ata6YPOlfgYbuHgXyhnlLt4TW"
              "~OTUDHzDHwccH-WJ12nrSf7rTHoax8zz6SSZszHnLsWz~K-j8FL5XzEkezIggBrmxA__&Key-Pair-Id=APKAIEYUVEN4EVB2OKEQ");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.portText),"");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.scheme),"https");
    uriFreeUriMembersA(&uri);
}

TEST(HtmlParserTest, ParseStreamjaHtml)
{
    std::filesystem::path streamable_html(RDTEST_DATA_FOLDER);
    streamable_html /= "streamja.html";
    HtmlParser parser(streamable_html);
    auto url = parser.getMediaLink("streamja.com");
    UriUriA uri;
    auto uriResult = uriParseSingleUriA(&uri, url.url.c_str(), nullptr);
    ASSERT_EQ(uriResult,URI_SUCCESS);

    EXPECT_FALSE(url.url.empty());
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.hostText),"tiger.cdnja.co");
    auto path = uri.pathHead;
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(path->text),"v");
    path = path->next;
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(path->text),"pa");
    path = path->next;
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(path->text),"pAvQ5.mp4");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.query),"secure=FXsFXIuHvvz7UJp8hVF1gA&expires=1607826600");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.portText),"");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.scheme),"https");
    uriFreeUriMembersA(&uri);
}

TEST(HtmlParserTest, ParseYoutubeHtml)
{
    std::filesystem::path streamable_html(RDTEST_DATA_FOLDER);
    streamable_html /= "youtube.html";
    HtmlParser parser(streamable_html);
    auto url = parser.getMediaLink("youtube.com");
    UriUriA uri;
    auto uriResult = uriParseSingleUriA(&uri, url.url.c_str(), nullptr);
    ASSERT_EQ(uriResult,URI_SUCCESS);

    EXPECT_FALSE(url.url.empty());
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.hostText),"r3---sn-cxaaj5o5q5-t34e.googlevideo.com");
    auto path = uri.pathHead;
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(path->text),"videoplayback");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.portText),"");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.scheme),"https");
    uriFreeUriMembersA(&uri);
}
TEST(HtmlParserTest, ParseYoutu_beHtml)
{
    std::filesystem::path streamable_html(RDTEST_DATA_FOLDER);
    streamable_html /= "youtu.be.html";
    HtmlParser parser(streamable_html);
    auto url = parser.getMediaLink("youtu.be");
    UriUriA uri;
    auto uriResult = uriParseSingleUriA(&uri, url.url.c_str(), nullptr);
    ASSERT_EQ(uriResult,URI_SUCCESS);

    EXPECT_FALSE(url.url.empty());
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.hostText),"r3---sn-cxaaj5o5q5-t34e.googlevideo.com");
    auto path = uri.pathHead;
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(path->text),"videoplayback");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.portText),"");
    EXPECT_EQ(MAKE_URI_TEXT_RANGE_STRING(uri.scheme),"https");
}
