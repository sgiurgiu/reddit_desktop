#include "gtest/gtest.h"
#include <boost/url.hpp>


TEST(UtilsTest, ParseUrl)
{
    boost::url_view urlParts("http://example.com/test?abc=123");
    EXPECT_EQ(urlParts.scheme().to_string(),"http");
    EXPECT_EQ(urlParts.encoded_host().to_string(),"example.com");
    EXPECT_EQ(urlParts.encoded_path().to_string(),"/test");
    EXPECT_EQ(urlParts.encoded_query().to_string(),"abc=123");
    EXPECT_EQ(urlParts.port().to_string(),"");
}

TEST(UtilsTest, ParseUrlWithPort)
{
    boost::url_view urlParts("http://example.com:8080/test?abc=123");
    EXPECT_EQ(urlParts.scheme().to_string(),"http");
    EXPECT_EQ(urlParts.encoded_host().to_string(),"example.com");
    EXPECT_EQ(urlParts.encoded_path().to_string(),"/test");
    EXPECT_EQ(urlParts.encoded_query().to_string(),"abc=123");
    EXPECT_EQ(urlParts.port().to_string(),"8080");
}

TEST(UtilsTest, ParseUrlNoScheme)
{
    boost::url_view urlParts("//example.com/test?abc=123");
    EXPECT_EQ(urlParts.scheme().to_string(),"");
    EXPECT_EQ(urlParts.encoded_host().to_string(),"example.com");
    EXPECT_EQ(urlParts.encoded_path().to_string(),"/test");
    EXPECT_EQ(urlParts.encoded_query().to_string(),"abc=123");
    EXPECT_EQ(urlParts.port().to_string(),"");
}
