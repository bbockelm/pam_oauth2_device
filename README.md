# PAM module for OAuth 2.0 Device flow

This is a PAM module that lets you log in via SSH to servers using OpenID Connect credentials, instead of SSH Keys or a username and password combination.

It uses the OAuth2 Device Flow, which means that during the login process, you will click a link and log in to your OpenID Connect Provider, which will then authenticate you for the SSH session. 

This module will then check if you're in the right group(s) or have a specified username, and allow or deny access.

A demo video is avaliable here: https://drive.google.com/file/d/1WzDRL0RFDXfvUgabbXNzBppV-DKXyUN1/view?usp=sharing

This code was originally developed by [Mazarykova Univerzita](https://github.com/ICS-MU/pam_oauth2_device) and [this branch](https://github.com/stfc/pam_oauth2_device) has been refactored by UKRI-STFC as a part of the [IRIS](https://www.iris.ac.uk/) activity.  This work was funded by [STFC](https://www.ukri.org/councils/stfc/), a part of [UK Research and Innovation (UKRI)](https://www.ukri.org/).


## Build

The upstream build uses basic `make` and we have stuck with this for compatibility reasons.
The two basic targets are `make` and `make test`; the latter builds the tests and
runs them, starting the OAuth2 mock server they need for the duration of the run.

* Note you can build RPMs and DEBs, though currently they are designed for CentOS7 and Ubuntu 18.04 respectively.
  * The build currently requires Docker (or compatible)


### Development container

The repository ships a dev container (see [`.devcontainer`](.devcontainer)) with
the compiler, the PAM/curl/LDAP headers, GoogleTest and `pamtester` already
installed.  Open the repository in VS Code and choose *Reopen in Container*, or
use the `devcontainer` CLI:

```
devcontainer up --workspace-folder .
devcontainer exec --workspace-folder . make test
```

It is based on AlmaLinux 9 to stay close to the platforms the module is packaged
for; build with `--build-arg IMAGE_BASE=...` to target a different EL release.


### Manual build dependencies (AlmaLinux/Rocky 8 and 9)

```
dnf install -y epel-release
dnf config-manager --set-enabled powertools   # EL8 only; gtest-devel lives there
dnf install -y gcc gcc-c++ make libcurl-devel openldap-devel pam-devel gtest-devel python3
```


### Manual Build on Scientific Linux or CentOS7

```
yum install epel-release
yum install openldap-devel
yum install libcurl-devel
yum install pam-devel
yum install libldb-devel
yum install http://ftp.scientificlinux.org/linux/scientific/7x/external_products/softwarecollections/yum-conf-softwarecollections-2.0-1.el7.noarch.rpm
yum install devtoolset-8 # this is needed as we need a more up to date g++ version than is supplied by default in SL repos.
scl enable devtoolset-8 bash
git clone https://github.com/stfc/pam_oauth2_device.git
cd pam_oauth2_device/
make
cp pam_oauth2_device.so /lib64/security/pam_oauth2_device.so
cp config_template.json config.json
```

### Build on Debian 10/Ubuntu Focal (20.04)


## Installation

To install the module, copy `pam_oauth2_device.so` into the PAM modules directory (usually with permissions 0755).

On Debian-based systems, this would be `/lib/x86_64-linux-gnu/security` whereas CentOS and related flavours would use `/usr/lib64/security`.  If in doubt, check `dpkg --L libpam-modules` or `rpm -ql pam` respectively.


## Configuration

Although JSON is not ideal for configuration files (it's a bit picky about syntax and it's hard to add comments), we now
support splitting the configuration file into several segments (all of which should be JSON "objects" at the outermost
level).  This is useful for maintaining parts of the file such as client secrets or user maps in separate files.

### Splitting configuration into several files

In any place where a JSON object (the curly braces construct) is expected in the configuration, a JSON string can be
placed instead with a filename in the string; this file will be loaded and will serve as the object in question and must
have the format expected in the configuration template.  This can be nested to any depth required.  It is strongly
recommended to use full pathnames inside these strings.

For example, if the normal configuration file looks like this (obviously a real configuration file would have more stuff
in it):

```
{
  "oauth": {
    "client": {
      "id": "abcd",
      "secret": "meetmeinstlouis"
  },
  "scope": "openid profile"
}
```
and the system administrator decides to store the client object in a separate file, they can do it as follows:

```
{
  "oauth": {
    "client": "/etc/pam_oauth2_device/client.json",
  "scope": "openid profile"
}
```
The `client.json` file should then look like this:
```
{
 "id": "abcd",
 "secret": "meetmeinstlouis"
}
```

In contrast, the `scope` cannot be split off into a separate file as a string is expected as value for the `scope` parameter.

### User names

Usernames are mentioned several times in this document and could probably get a bit confusing.  This section attempts to give a short explanation.

For every user there are *three* usernames, which can be distinct.

The **local user name** is the name the user uses in the ssh login, as in `ssh fred@example.com` where the local user name is `fred`.  This is the name that is passed into the PAM module for authentication.

The **remote user name** is the corresponding name for the user as held by the IAM system.  Once the user has successfully authenticated to IAM, IAM publishes a "userinfo" structure with the user's name and email address and other attributes that IAM can assert.  Within this structure, the PAM module can pick an attribute to use as the remote user name (using the `username_attribute` option).

The **account name** is the name of the local Unix account that the user is mapped into once they have authenticated.  By default, it is the same as the local user name.

### Summary of Configuration Options

The template `config_template.json` should give an outline of the configuration file.

The configuration should be installed at `/etc/pam_oauth2_device/config.json` (a future release should make it configurable, to allow different modules to coexist).

As the name suggests, the file is in JSON, so it is recommended to check it with a JSON validator like `jq` after editing it (the PAM module will refuse to load an invalid JSON file, but you will not see this error till runtime.)

Thus, at the top level, there is a single object with a number of entries, described as "sections" in the table:

#### Table 1: Configuring Authentication Flow

| Section | Entry | Type | Req'd | Description | Notes |
| --- | --- | --- | --- | --- | --- |
| oauth | | Object | Y | | |
| oauth | client | Object | Y | Contains "id" and "secret" | |
| oauth | scope | String | Y | OIDC scope | Note 1 |
| oauth | device\_endpoint | String | Y | Device endpoint | https://${url}/devicecode |
| oauth | token\_endpoint | String | Y | Token endpoint | https://${url}/token |
| oauth | userinfo\_endpoint | String | Y | Userinfo | https://${url}/userinfo |
| oauth | username\_attribute | String | Y | Attribute for remote username | |
| oauth | name\_attribute | String | Y | Attribute for a user's full name | |
| oauth | local\_username\_suffix | String | Y | See usernames | |
| tls | | Object | Y | | |
| tls | ca\_bundle | String | N | Concatenated list of trust anchors | Note 2 |
| tls | ca\_path | String | N | Directory with trust anchors | Note 2 |
| qr | | Object | N | | |
| qr | error\_correction\_level | Int | Y | QR code | Note 3 |

Notes:

1 The string value should have a _space separated_ list of scopes which must include `offline_access`
2 If present, the "ca\_bundle" must be a file with PEM-formatted trust anchors (CA certificates) concatenated together.
   * "ca\_path" works only with OpenSSL
   * On the target system, use `curl -V` to see whether curl uses OpenSSL or NSS
   * If both ca\_path and ca\_bundle are present, the latter takes precedence
3 The QR code section is optional but if present, it must have the error correction level defined.  Permitted values are 1 (low), 2 (medium), 3 (high) or -1 (disabled).  If the section is missing, the QR code is disabled.
4 The "${url}" above would be the URL (hostname) of your OpenID Provider.  Its host certificate must be valid when checked against the CA bundle (see item 2)

#### Table 2: Configuring Authorisation Flow

No authorisation section is required, although if included there will be necessary entries in the section.  However, if no authorisation section is provided, the user will not be able to log in.

This part of the module functionality carries a lot of legacy stuff; see the Authorisation section for discussion.

| Section | Entry | Type | Req'd | Description | Notes |
| --- | --- | --- | --- | --- | --- |
| ldap | | Object | N | LDAP configuration | |
| ldap | host | String | Y | LDAP URL | |
| ldap | basedn | String | Y | Base DN | Note 1 |
| ldap | user | String | Y | username | Note 2 |
| ldap | passwd | String | Y | password | Note 2 |
| ldap | scope | String | N | | Note 3 |
| ldap | preauth | String | N | | |
| ldap | filter | String | Y | | |
| ldap | attr | String | Y | | |
| group | | Object | N | | |
| group | access | boolean | Y | Whether to use this section | |
| group | service\_name | String | Y | | Note 4 |
| cloud | | Object | N | | |
| cloud | access | boolean | Y | Whether to use this section | |
| cloud | endpoint | String | Y | | Note 5 |
| cloud | username | String | Y | endpoint username | Note 5 |
| cloud | metadata\_file | String | Y | | Note 5 |
| users | | Object | N | User Mapping section | Note 6 |

1 The base DN, least significant RDN first
2 Username and password are for authentication to the LDAP server, if used; if not used, just leave them as empty strings
3 scope is one of 'sub'/'subtree', 'one'/'onelevel' or 'base'/'baseobject'
  - If the LDAP implementation supports 'subordinate' or 'children' (these are synonymous) then these are available as scopes as well
  - The default is 'sub'
4 The service name is a string, which should name a group
5 The 'cloud' section implements a callout to a server to fetch a group membership file
6 The 'users' section provides mappings from the username attribute (selected with username\_attribute) to a local user id.

### Bypass

The module provides a feature for bypassing authentication altogether, letting the process fall through to the next PAM
module in the stack (provided the PAM authentication is configured correctly; see the HOWTO for further details.)  A
typical use case is to treat root logins separately.

There are currently two ways of bypassing; one is an LDAP lookup based on the "preauth" query, and the other is a
special 'users' section where the remote username is the magic string `*bypass*`; if the local username is in this
section, the PAM module is bypassed.  See the HOWTO for further details.

### Deprecated?

- Future releases should change the `client_debug` to loglevel.
  - Additionally, adding `debug` to the PAM config should enable debug, as expected for a PAM module
- The metadata file called `project_id` currently has a backwards compatible default of `/mnt/context/openstack/latest/meta_data.json`


## Multi-factor authentication

Some OpenID Connect providers record whether the user's identity provider
performed MFA.  CILogon reports it as an [Authentication Context Class
Reference](https://www.cilogon.org/oidc): an `acr` claim whose value is
`https://refeds.org/profile/mfa`.  The **mfa** section lets you require a
second factor from the PAM stack when that claim is absent.

```json
"mfa": {
    "acr_values": ["https://refeds.org/profile/mfa"],
    "amr_values": [],
    "if_absent": "second_factor"
}
```

**acr_values** - values of the userinfo `acr` claim that count as MFA having
been performed.  **amr_values** - values of the `amr` claim that count, for
providers that report the methods used rather than a context class.  Either
list may be a single string instead of an array; a match in either satisfies
the policy.

**if_absent** - what to do when neither matched:

  * `ignore` - do not look at the claims at all.  This is the default, so
    existing configurations are unaffected.
  * `second_factor` - authentication succeeded, but `pam_sm_authenticate`
    returns `PAM_CRED_INSUFFICIENT` so the PAM stack can run a second factor.
  * `deny` - refuse the login.

Setting `if_absent` to anything but `ignore` without giving any `acr_values` or
`amr_values` is a configuration error: the policy would otherwise apply to every
login.  The MFA check runs *after* authorisation, so a user who would be refused
anyway is told no rather than being sent round a second factor first.

### A caveat: MFA now versus MFA at some point

These claims describe the authentication that started the provider's SSO
session, which is not necessarily this login, and the userinfo response carries
no `auth_time` with which to tell the difference.

An SSO session at the provider can outlive not just the login but a change in
the user's MFA enrolment: enabling 2FA on an account and repeating the device
flow can keep returning the pre-2FA answer until that session ends.  The session
that goes stale is the OpenID Connect provider's, so logging out of the identity
provider behind it does not necessarily clear it.

So this feature answers "did this user do MFA at some point in their current
provider session?", not "was this user challenged just now".  If you need the
stronger property, this is not enough on its own, and the `auth_time` claim
needed to bound it appears only in the ID token, which the module does not read.

### Requiring TOTP from the PAM stack

`second_factor` reports its result through the return code, so the PAM stack
decides what the second factor actually is.  With
[pam_oath](https://www.nongnu.org/oath-toolkit/pam_oath.html):

```
auth [success=done cred_insufficient=ignore ignore=ignore default=die] pam_oauth2_device.so /etc/pam_oauth2_device/config.json
auth requisite  pam_oath.so usersfile=/etc/users.oath window=10 digits=6
auth required   pam_permit.so
```

Read the first line as: if the provider recorded MFA, the stack is finished and
succeeds; if it did not, contribute nothing and carry on to `pam_oath`; on
anything else, fail immediately.  `pam_google_authenticator`, `pam_yubico` and
the like drop into the same slot.

Two of those control values are easy to get wrong, in ways that are not obvious
from reading the stack:

  * `cred_insufficient` must be **`ignore`**, not `ok`.  `ok` sets the stack's
    status to `PAM_CRED_INSUFFICIENT`, and a later success cannot overwrite a
    non-success status - so the second factor would pass and the login would
    still be refused.
  * `ignore` must be **`ignore`**, not a jump.  Accounts listed under
    `*bypass*` make the module return `PAM_IGNORE`, and a jump would carry them
    over the second factor and onto whatever follows it - with the stack above,
    straight onto `pam_permit`, letting those accounts in with no credential at
    all.

With `ignore=ignore`, a `*bypass*` account falls through to the modules after
this one and has to satisfy them, exactly as it would if this module were not
in the stack.  In the stack above that means it must pass `pam_oath`.  If those
accounts should authenticate some other way, route them before this module
reaches them:

```
auth [success=done default=ignore] pam_succeed_if.so user in root:emergency quiet
auth substack   password-auth
auth [success=done cred_insufficient=ignore ignore=ignore default=die] pam_oauth2_device.so /etc/pam_oauth2_device/config.json
auth requisite  pam_oath.so usersfile=/etc/users.oath window=10 digits=6
auth required   pam_permit.so
```

Whatever you settle on, check it before putting it in front of sshd.  Both
stacks above are exercised by `test/pam_stack_test.sh`, which drives them with
`pamtester` and a stub module.

### Checking what your provider actually sends

Whether an `acr` claim appears at all depends on the identity provider the user
picks, not just on CILogon, and CILogon does not advertise `acr_values_supported`
in its discovery document - so you cannot ask for MFA, only observe whether it
happened.  Before configuring a policy, run
[`util/oidc-probe/oidc-probe.py`](util/oidc-probe/oidc-probe.py) against your
client:

```
./util/oidc-probe/oidc-probe.py /etc/pam_oauth2_device/config.json
```

It runs one device flow and reports which of the four possible sources - the ID
token, the access token, the userinfo endpoint and token introspection - carry
`acr`, `amr` and `auth_time`.  Do it twice, once with an IdP that does MFA and
once with one that does not, and check that the claim really does appear and
disappear.  The script prints the claims of the ID token as well as the
userinfo response, so treat its output as sensitive.

Which source carries what (measured against CILogon, September 2026):

| source | `acr` / `amr` | `auth_time` |
| --- | --- | --- |
| userinfo endpoint | when the IdP asserts one | no |
| ID token | when the IdP asserts one | yes |
| access token | opaque, not a JWT | - |
| introspection endpoint | no | no |

The module reads the **userinfo** response, which it already fetches to get the
username and groups, so the MFA check costs no extra request.  Note that `acr`
is an ID Token claim in OpenID Connect Core rather than a userinfo claim, so a
provider that is strict about this may not return it from userinfo even though
CILogon does; the probe described below will tell you.

Identity providers differ over what they report, and the four measured behind a
single CILogon client covered four distinct shapes:

| what the provider reports | example |
| --- | --- |
| `acr` naming the REFEDS MFA profile | `https://refeds.org/profile/mfa` |
| `acr` naming some other context, even though MFA was used | `urn:oasis:names:tc:SAML:2.0:ac:classes:PasswordProtectedTransport` |
| no `acr`, but an `amr` listing the methods | `"pwd"`, becoming `"mfa"` once the account enabled 2FA |
| neither claim | - |

Three things follow, and they set the expectations you should have of this
feature:

  * A policy has to match `acr` and `amr` *values*, and cope with both being
    missing.  Testing whether a claim exists would accept the second row, where
    MFA was in use but not reported as such; reading one unconditionally would
    break on the fourth.
  * `acr` is not the only signal.  Some providers send only `amr`, which is why
    `amr_values` exists.  Both claims are accepted as a string or as an array
    of strings, because CILogon sends `amr` as a bare string even though OpenID
    Connect Core specifies an array.
  * An identity provider that performs MFA does not necessarily say so in
    REFEDS terms, and CILogon does not advertise `acr_values_supported`, so the
    context cannot be requested - only observed.  Expect users at such providers
    to be sent to the second factor even though they have already used one, and
    check your own user population with the probe before promising anyone a
    smooth path.

A configuration covering both of the providers that do report it:

```json
"mfa": {
    "acr_values": ["https://refeds.org/profile/mfa"],
    "amr_values": ["mfa", "otp"],
    "if_absent": "second_factor"
}
```

### Troubleshooting

Everything below is logged to `authpriv`, so on EL it lands in
`/var/log/secure`.  Each line is tagged with the PAM service and the process,
so `grep sshd /var/log/secure` around the time of the login is the place to
start.

A login that was refused, or unexpectedly sent to a second factor, logs what
the provider said *and* what was configured:

```
provider did not record MFA: acr <absent>, amr pwd; \
    configured acr_values [https://refeds.org/profile/mfa], amr_values [mfa]
requiring a second factor: returning PAM_CRED_INSUFFICIENT. ...
```

  * **`acr <absent>, amr <absent>`** - the identity provider reported nothing
    about how the user authenticated.  Nothing to configure; those users always
    get the second factor.  Google is like this.
  * **A value you did not expect** - add it to `acr_values` or `amr_values` if
    it does mean MFA.  Confirm with the probe first, on an account you know is
    not using MFA, that the value actually changes.
  * **`returning PAM_CRED_INSUFFICIENT` followed by a failed login and nothing
    else** - the PAM stack is not handling `cred_insufficient`.  That is the
    module asking for a second factor and the stack treating it as a refusal;
    check the control flags against the stack above, and against
    `test/pam_stack_test.sh`.
  * **The user did use MFA, and the module still asked for a second factor** -
    either the provider reported it as a value that is not in your lists (see
    above), or the claim describes an older authentication in the same provider
    session, which this feature cannot distinguish.

Setting `"client_debug": true` logs the whole userinfo response, which contains
the user's identity attributes as well as the claims, so turn it off again
afterwards.


## Authorisation

The major refactoring of the module in Sep 2021 preserved (and bugfixed) the existing authorisation functionality.  However, the user should be warned that it is subject to revision, but generally preserving backward functionality if possible.

### Comprehensive Example

As above, user Fred Bloggs logs in with `ssh fred@example.com`.  The host at `example.com` asks Fred to authenticate.  Once successfully, it calls out to IAM to obtain the userinfo structure.  From this it picks the attribute specified with `username_attribute` in the configuration, `preferred_username`, say.  Let's say the value of `preferred_username` of Fred's userinfo structure is `bloggs`.  Additionally, the userinfo structure contains the list of groups "users", "iris" and "cloud".

Throughout the rest of this section, it is assumed that Fred has authenticated successfully to IAM.

If the **cloud** section is configured and `access` is true, a local file configured as the `metadata_file` is read.  This file should contain the structure

```
{"project\_id": "fleeps"}
```

The module adds the string `fleeps` to the endpoint (with a slash) and calls the server (with *no* client authentication) to see what is at the endpoint.  It expects a JSON structure as response, structured like

```
{"groups": ["wop", "fap", "foo", "users"]}
```

If one of these groups matches Fred's groups as returned by the userinfo structure (it does here, "users"), then Fred is considered authorised.  An additional check is made whether the local username plus suffix equals the remote username.  If this check and the cloud group membership both pass, then Fred is considered authorised (the username check would fail in this example, because no suffix can make 'fred' equal to 'bloggs'.)


If the **group** section is configured and `access` is set to true, a check will be made whether the configured value for `service_name` is one of Fred's groups.  Note the service name is single valued.  Additionally, as for the cloud section, the local username plus suffix must equal the remote username.

If Fred's remote username were `fred_fleeps` then it *would* match the local username (`fred`) if the suffix were configured as `_fleeps`.

If Fred is not authorised through  the cloud or group sections, either because the check failed or they were not enabled, then a configured usermap is consulted.  This is written straight into the configuration file (it should probably be in its own file at some point), so would be suitable only for a smallish number of users.

This usermap is in the **users* section which expects a JSON object mapping the *remote* username to an array of permissible local usernames. Thus, the same user could have multiple local logins using this method.  If the local username is found here, Fred is considered authorised.  No suffix is used in this section.

If Fred is not authorised through any of these methods, the module falls back to an LDAP lookup (if the **ldap** section is configured).  The LDAP query takes a configured filter and substitutes the *remote* username for a `%s` part of the filter, and queries a configured attribute.

If the filter is `(&(objectClass=user)(cn=%s))` then `bloggs` is substituted in our example, and a target attribute (configured with `attr`) is queried from the LDAP server.  For example, if `attr` has the value `uid` then the equivalent of

```
ldapsearch -x -H ldap://host -b base '(&(objectClass=user)(cn=bloggs))' uid
```

is run and the result is compared with the local username, `fred`.  If these match (no suffix is used), then Fred is again considered authorised.


## SSH Configuration

You MUST edit the configuration before this module will work!

Make sure the module works correctly before changing your SSH config or you may be locked out!  See Testing below.

Edit `/etc/pam.d/sshd` and comment out the other `auth` sections (unless you need MFA or something else).

```
auth required pam_oauth2_device.so
```

Edit `/etc/ssh/sshd_config` and make sure that the following configuration options are set

```
ChallengeResponseAuthentication yes
UsePAM yes
```

```
systemctl restart sshd
```

## Testing the module works

You are advised to do this before making changes to your main SSH config.  There are two tests to do which are recommended to do in the order described here.

### Preparing for the tests

It is recommended that you create a (hard or sym) link to your `sshd` called (for example) `pamsshd`, e.g. `/usr/local/sbin/pamsshd`.  This means you can have a PAM configuration for `pamsshd` which is different from the normal `sshd`.

In this case you can copy `/etc/pam.d/sshd` to `/etc/pam.d/pamsshd` and edit the latter, leaving the former to log you back into the system if something goes wrong.  Also copy `/etc/ssh/sshd_config` to `/etc/ssh/pamsshd_config` so you can edit the configuration for `pamsshd` separately.

Note that testing *requires* that you install the module in the system location and you have the configuration set up in `/etc/pam_oauth2_device/config.json` and `/etc/pam.d/` and `/etc/ssh/`.

### Test 1: pam tester

Follow instructions above, and additionally install `pamtester`.

Run
```
pamtester -v pamtester localusername authenticate
```
and follow the onscreen prompts.  Here, `localusername` refers to your local user name so replace it with whatever your name is.

You can check `/var/log/secure` or `/var/log/auth.log` to find what's wrong if there are errors authenticating.  While the module
uses syslog, syslog is normally set up to log PAM stuff into one of these files.

### Test 2: sshd

While pamtester tests the authenticate section, you should try a proper ssh login from another host.  If you created `pamsshd` as above (and copied configuration as described above), start it with

```
/usr/local/sbin/pamsshd -f /etc/ssh/pamsshd_config -p 2222 -d
```

This should start `sshd` with the name `pamsshd` listening on port 2222.  Now try to log in from another host (bearing in mind the port should be open for incoming tcp).  On the other host, run `ssh -p 2222 localusername@myhost` where `localusername` is the local user name and `myhost` is the host running `pamsshd`.

Again check the logs as in the previous tests.


