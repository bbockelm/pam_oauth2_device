#ifndef PAM_OAUTH2_DEVICE_CONFIG_HPP
#define PAM_OAUTH2_DEVICE_CONFIG_HPP

#include <map>
#include <set>
#include <string>

class Config
{
public:
    /*! @brief What to do when the provider did not record that MFA was performed.
     *
     * IGNORE         - do not look at the acr/amr claims at all (the default,
     *                  and what every existing configuration gets).
     * SECOND_FACTOR  - authentication succeeded, but pam_sm_authenticate returns
     *                  PAM_CRED_INSUFFICIENT so the PAM stack can run a second
     *                  factor.  See the "mfa" section of README.md.
     * DENY           - refuse the login outright.
     */
    enum class mfa_policy_t { IGNORE, SECOND_FACTOR, DENY };

    void load(const char *path);
    std::string client_id,
        client_secret,
        scope,
        device_endpoint,
        token_endpoint,
        userinfo_endpoint,
        username_attribute,
        name_attribute,
        ldap_host,
        ldap_basedn,
	ldap_scope,
        ldap_user,
        ldap_passwd,
	ldap_preauth,
        ldap_filter,
        ldap_attr,
	tls_ca_path,
	tls_ca_bundle,
        group_service_name,
        cloud_endpoint,
        cloud_username,
        local_username_suffix,
        metadata_file;
    // Defaults mirror the ones applied by Config::load(); they matter for
    // Config objects that are never load()ed, e.g. in the unit tests.
    int qr_error_correction_level = -1;
    bool group_access = false,
         cloud_access = false,
         group_and_username_access = false,
         http_basic_auth = true,
         client_debug = false;
    std::map<std::string, std::set<std::string>> usermap;

    //! Values of the userinfo acr claim that count as MFA having been done.
    std::set<std::string> mfa_acr_values;
    //! Values of the userinfo amr claim that count as MFA having been done.
    std::set<std::string> mfa_amr_values;
    mfa_policy_t mfa_if_absent = mfa_policy_t::IGNORE;
};

#endif // PAM_OAUTH2_DEVICE_CONFIG_HPP
