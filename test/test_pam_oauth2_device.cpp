#include "gtest/gtest.h"
#include "pam_oauth2_device.hpp"


#define DEVICE_ENDPOINT "http://localhost:8042/devicecode"
#define TOKEN_ENDPOINT "http://localhost:8042/token"
#define USERINFO_ENDPOINT "http://localhost:8042/userinfo"
#define USERNAME_ATTRIBUTE "preferred_username"
#define NAME_ATTRIBUTE "name"
#define CLIENT_ID "client_id"
#define CLIENT_SECRET "NDVmODY1ZDczMGIyMTM1MWFlYWM2NmYw"
#define SCOPE "openid profile"
#define USER_CODE "QWERTY"
#define DEVICE_CODE "e1e9b7be-e720-467e-bbe1-5c382356e4a9"
#define ACCESS_TOKEN "ZjBhNTQxYzEzMGQwNWU1OWUxMDhkMTM5"
#define VERIFICATION_URL "http://localhost:8042/oidc/device"

namespace
{

// These exercise the OAuth2 endpoints against test/mock_server.py, which
// test/run_tests.sh starts for the duration of the run.

//! A logger that discards everything: there is no pam handle under test.
pam_oauth2_log test_logger()
{
    return pam_oauth2_log(nullptr, pam_oauth2_log::log_level_t::DEBUG);
}

/*! @brief A config holding the mock server's client credentials.
 *
 * pam_oauth2_curl takes the credentials it authenticates with from the Config,
 * not from the client_id/client_secret arguments of the functions below, so
 * the Config has to carry them just as it does in pam_sm_authenticate().
 */
Config test_config()
{
    Config config;
    config.client_id = CLIENT_ID;
    config.client_secret = CLIENT_SECRET;
    config.scope = SCOPE;
    return config;
}

TEST(PamTest, Device)
{
    Config config = test_config();
    pam_oauth2_log logger = test_logger();
    DeviceAuthResponse response;
    make_authorization_request(config,
                               logger,
                               config.client_id,
                               config.client_secret,
                               config.scope,
                               DEVICE_ENDPOINT,
                               &response);
    EXPECT_EQ(response.user_code, USER_CODE);
    EXPECT_EQ(response.device_code, DEVICE_CODE);
    EXPECT_EQ(response.verification_uri, VERIFICATION_URL);
    EXPECT_EQ(response.verification_uri_complete,
              std::string(VERIFICATION_URL) + "?user_code=" + DEVICE_CODE);
}

TEST(PamTest, Token)
{
    Config config = test_config();
    pam_oauth2_log logger = test_logger();
    std::string token;
    poll_for_token(config,
                   logger,
                   config.client_id,
                   config.client_secret,
                   TOKEN_ENDPOINT,
                   DEVICE_CODE,
                   token);
    EXPECT_EQ(token, ACCESS_TOKEN);
}

TEST(PamTest, Userinfo)
{
    Config config = test_config();
    pam_oauth2_log logger = test_logger();
    Userinfo userinfo = get_userinfo(config,
                                     logger,
                                     USERINFO_ENDPOINT,
                                     ACCESS_TOKEN,
                                     USERNAME_ATTRIBUTE,
                                     NAME_ATTRIBUTE);
    EXPECT_EQ(userinfo.sub(), "YzQ4YWIzMzJhZjc5OWFkMzgwNmEwM2M5");
    EXPECT_EQ(userinfo.username(), "jdoe");
    EXPECT_EQ(userinfo.name(), "Joe Doe");
}

} // namespace
