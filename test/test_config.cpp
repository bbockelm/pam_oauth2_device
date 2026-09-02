#include "gtest/gtest.h"
#include "include/config.hpp"
#include "include/nlohmann/json.hpp"

#define CLIENT_ID "client_id"

using json = nlohmann::json;

namespace
{

TEST(ConfigTest, MissingFile)
{
    Config config;
    try
    {
        config.load("data/missing.json");
        FAIL() << "Expected std::runtime_error";
    }
    catch (std::runtime_error const &e)
    {
        EXPECT_STREQ("File not found: data/missing.json", e.what());
    }
}

TEST(ConfigTest, WrongFormat)
{
    Config config;
    ASSERT_THROW(config.load("data/template_wrong.json"), json::parse_error);
}

TEST(ConfigTest, Empty)
{
    Config config;
    ASSERT_THROW(config.load("data/template_empty.json"), json::out_of_range);
}

TEST(ConfigTest, NoLdap)
{
    Config config;
    config.load("data/template_noldap.json");
    EXPECT_EQ(config.client_id, CLIENT_ID);
    EXPECT_TRUE(config.ldap_host.empty());
}

TEST(ConfigTest, Full)
{
    Config config;
    config.load("../config_template.json");
    EXPECT_EQ(config.client_id, CLIENT_ID);
    EXPECT_EQ(config.ldap_host, "ldaps://ldap-server:636");
    EXPECT_EQ(config.usermap["*bypass*"].count("root"), 1);
    EXPECT_EQ(config.usermap["provider_user_id_1"].count("bob"), 1);
    EXPECT_EQ(config.usermap["provider_user_id_2"].count("mike"), 1);
    EXPECT_EQ(config.usermap.size(), 3);
    EXPECT_EQ(config.qr_error_correction_level, 0);
}

} // namespace