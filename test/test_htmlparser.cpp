#include "gtest/gtest.h"
#include <boost/url.hpp>
#include <filesystem>
#include "htmlparser.h"

TEST(HtmlParserTest, ParseStreamableHtml)
{
    std::filesystem::path streamable_html(RDTEST_DATA_FOLDER);
    streamable_html /= "streamable.html";
    HtmlParser parser(streamable_html);
    auto url = parser.getVideoUrl("streamable.com");
    boost::url_view urlParts(url);
    EXPECT_FALSE(url.empty());
    EXPECT_EQ(urlParts.encoded_host().to_string(),"cdn-cf-east.streamable.com");
    EXPECT_EQ(urlParts.encoded_path().to_string(),"/video/mp4/sh7m2g.mp4");
    EXPECT_EQ(urlParts.encoded_query().to_string(),"Expires=1608061320&Signature=Hr~3Vu0BrnNckKtaY8R-rqlTVwB00WKF1s0F6RKUiXu1Qx9dCJ5wvaM83d1IFuPDkSvGawrwUQEY"
              "s0-m7XGqL8GzUMCt-OXAr2C7gzoJICIii1BQX3vNBGS2-mDMnFCm-G26bZg1qE6-ToKU~nyY1o4JR85IETAi1UHvNVJADGFoW-G6z1A7nfxmm3S8D4vPMWr9mK5hkOoWIUa9pnJiTCF1ZXfVxLXhDKSqZXWVcJJKJxSPOJiHAM8Ph29Ata6YPOlfgYbuHgXyhnlLt4TW"
              "~OTUDHzDHwccH-WJ12nrSf7rTHoax8zz6SSZszHnLsWz~K-j8FL5XzEkezIggBrmxA__&Key-Pair-Id=APKAIEYUVEN4EVB2OKEQ");
    EXPECT_EQ(urlParts.port().to_string(),"");
    EXPECT_EQ(urlParts.scheme().to_string(),"https");
}

TEST(HtmlParserTest, ParseStreamjaHtml)
{
    std::filesystem::path streamable_html(RDTEST_DATA_FOLDER);
    streamable_html /= "streamja.html";
    HtmlParser parser(streamable_html);
    auto url = parser.getVideoUrl("streamja.com");
    boost::url_view urlParts(url);
    EXPECT_FALSE(url.empty());
    EXPECT_EQ(urlParts.encoded_host().to_string(),"tiger.cdnja.co");
    EXPECT_EQ(urlParts.encoded_path().to_string(),"/v/pa/pAvQ5.mp4");
    EXPECT_EQ(urlParts.encoded_query().to_string(),"secure=FXsFXIuHvvz7UJp8hVF1gA&expires=1607826600");
    EXPECT_EQ(urlParts.port().to_string(),"");
    EXPECT_EQ(urlParts.scheme().to_string(),"https");
}
