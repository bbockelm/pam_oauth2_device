#ifndef PAM_OAUTH2_DEVICE_CONFIG_HPP
#define PAM_OAUTH2_DEVICE_CONFIG_HPP

#include <map>
#include <set>
#include <string>

class Config
{
public:
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
};

#endif // PAM_OAUTH2_DEVICE_CONFIG_HPP
