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

TEST(ConfigTest, MfaDefaultsToOff)
{
    // Every configuration written before the mfa section existed must keep
    // behaving exactly as it did.
    Config config;
    config.load("data/template_noldap.json");
    EXPECT_EQ(config.mfa_if_absent, Config::mfa_policy_t::IGNORE);
    EXPECT_TRUE(config.mfa_acr_values.empty());
    EXPECT_TRUE(config.mfa_amr_values.empty());
}

TEST(ConfigTest, Mfa)
{
    Config config;
    config.load("data/template_mfa.json");
    EXPECT_EQ(config.mfa_if_absent, Config::mfa_policy_t::SECOND_FACTOR);
    EXPECT_EQ(config.mfa_acr_values.count("https://refeds.org/profile/mfa"), 1);
    EXPECT_EQ(config.mfa_acr_values.size(), 1);
    EXPECT_EQ(config.mfa_amr_values.count("otp"), 1);
    EXPECT_EQ(config.mfa_amr_values.count("mfa"), 1);
    EXPECT_EQ(config.mfa_amr_values.size(), 2);
}

TEST(ConfigTest, MfaPolicyWithoutValuesIsRejected)
{
    // Asking for a policy without saying what counts as MFA would apply it to
    // every login; that is a mistake worth refusing to start on.
    Config config;
    ASSERT_THROW(config.load("data/template_mfa_novalues.json"), std::runtime_error);
}

TEST(ConfigTest, MfaValuesMayBeASingleString)
{
    // Documented as accepting either a string or an array.
    Config config;
    config.load("data/template_mfa_string.json");
    EXPECT_EQ(config.mfa_acr_values.count("https://refeds.org/profile/mfa"), 1);
    EXPECT_EQ(config.mfa_acr_values.size(), 1);
    EXPECT_EQ(config.mfa_amr_values.count("mfa"), 1);
}

TEST(ConfigTest, MfaRejectsMalformedSection)
{
    for (char const *fixture : {"data/template_mfa_badpolicy.json",
                                "data/template_mfa_badtype.json"})
    {
        Config config;
        ASSERT_THROW(config.load(fixture), std::runtime_error) << fixture;
    }
}

} // namespace