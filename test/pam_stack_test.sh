#!/bin/bash
# Exercises the PAM stacks documented in README.md.
#
# The control flags there are not self-evidently correct -- an earlier version
# of that snippet both let *bypass* accounts in with no credential and refused
# logins whose second factor had succeeded -- so they are checked rather than
# reasoned about.
#
# The module itself is replaced by a stub returning a chosen PAM code, because
# what is under test is the stack, not the OAuth2 flow.  Needs root, pamtester,
# and a compiler; skips cleanly without them.
set -u

SECURITY_DIR=${SECURITY_DIR:-/usr/lib64/security}

for tool in pamtester gcc; do
    if ! command -v $tool >/dev/null; then
        echo "SKIP: $tool not installed"
        exit 0
    fi
done
if [ "$(id -u)" != 0 ]; then
    echo "SKIP: needs root to install a PAM service"
    exit 0
fi
if [ ! -d "$SECURITY_DIR" ]; then
    echo "SKIP: no $SECURITY_DIR (set SECURITY_DIR)"
    exit 0
fi

work=$(mktemp -d)
trap 'rm -rf "$work" "$SECURITY_DIR/pam_stub_oauth2.so" /etc/pam.d/pam-oauth2-stacktest' EXIT

cat > "$work/stub.c" <<'EOF'
#include <security/pam_modules.h>
#include <string.h>
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags,
                                   int argc, const char **argv)
{
    if (argc > 0) {
        if (!strcmp(argv[0], "ignore"))            return PAM_IGNORE;
        if (!strcmp(argv[0], "cred_insufficient")) return PAM_CRED_INSUFFICIENT;
        if (!strcmp(argv[0], "success"))           return PAM_SUCCESS;
        if (!strcmp(argv[0], "auth_err"))          return PAM_AUTH_ERR;
    }
    return PAM_SERVICE_ERR;
}
PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags,
                              int argc, const char **argv)
{
    return PAM_SUCCESS;
}
EOF
gcc -fPIC -shared -o "$SECURITY_DIR/pam_stub_oauth2.so" "$work/stub.c" -lpam || {
    echo "FAIL: could not build the stub module"; exit 1; }

# The first line of the documented stack, with the stub standing in.
MODULE='auth [success=done cred_insufficient=ignore ignore=ignore default=die] pam_stub_oauth2.so'

failures=0
# check <expected pass|fail> <description> <stack lines...>
check() {
    local expected="$1" description="$2"; shift 2
    printf '%s\n' "$@" > /etc/pam.d/pam-oauth2-stacktest
    if pamtester pam-oauth2-stacktest root authenticate </dev/null >/dev/null 2>&1; then
        actual=pass
    else
        actual=fail
    fi
    if [ "$actual" = "$expected" ]; then
        printf '  ok       %s (%s)\n' "$description" "$actual"
    else
        printf '  NOT OK   %s: expected %s, got %s\n' "$description" "$expected" "$actual"
        failures=$((failures + 1))
    fi
}

echo "PAM stack from README.md:"
check pass "provider recorded MFA, second factor skipped" \
    "$MODULE success"           'auth requisite pam_deny.so'   'auth required pam_permit.so'
check pass "no MFA recorded, second factor succeeds" \
    "$MODULE cred_insufficient" 'auth requisite pam_permit.so' 'auth required pam_permit.so'
check fail "no MFA recorded, second factor fails" \
    "$MODULE cred_insufficient" 'auth requisite pam_deny.so'   'auth required pam_permit.so'
check fail "authentication itself failed" \
    "$MODULE auth_err"          'auth requisite pam_permit.so' 'auth required pam_permit.so'
# The one that matters most: a bypass account must not be waved through.
check fail "*bypass* account must still satisfy what follows" \
    "$MODULE ignore"            'auth requisite pam_deny.so'   'auth required pam_permit.so'
check pass "*bypass* account that does satisfy what follows" \
    "$MODULE ignore"            'auth requisite pam_permit.so' 'auth required pam_permit.so'

echo "Routing *bypass* accounts to their own stack:"
ROUTE='auth [success=done default=ignore] pam_succeed_if.so user in root quiet'
check pass "routed account authenticates before this module" \
    "$ROUTE" "$MODULE ignore" 'auth requisite pam_deny.so' 'auth required pam_permit.so'

if [ $failures -ne 0 ]; then
    echo "$failures PAM stack check(s) failed"
    exit 1
fi
echo "PAM stack checks passed"
