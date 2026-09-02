#!/usr/bin/env python3
"""Run the device flow once and show what the provider actually returns.

The point of this is the acr/amr claims: the module can only require a second
factor when the provider records whether MFA happened, and whether it does so
depends on the identity provider the user picks, not just on CILogon.  Run this
against a real client before configuring the "mfa" section, and log in once with
an IdP that does MFA and once with one that does not.

It reads the same config.json the PAM module uses, so it needs the client
secret; run it as a user that can read the file.  Nothing is written anywhere,
and the tokens are printed to stdout, so do not run it where the output is
logged.

  ./oidc-probe.py /etc/pam_oauth2_device/config.json
"""

import argparse
import base64
import json
import sys
import time
import urllib.error
import urllib.parse
import urllib.request


DEVICE_CODE_GRANT = 'urn:ietf:params:oauth:grant-type:device_code'


def post(url, params, client_id, client_secret, basic_auth):
    """POST a form, authenticating the way the module does."""
    data = dict(params)
    headers = {'Content-Type': 'application/x-www-form-urlencoded'}
    if basic_auth:
        raw = '{}:{}'.format(client_id, client_secret).encode()
        headers['Authorization'] = 'Basic ' + base64.b64encode(raw).decode()
    else:
        data['client_id'] = client_id
        data['client_secret'] = client_secret

    request = urllib.request.Request(
        url, data=urllib.parse.urlencode(data).encode(), headers=headers)
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return json.loads(response.read().decode())
    except urllib.error.HTTPError as e:
        body = e.read().decode(errors='replace')
        try:
            return json.loads(body)
        except ValueError:
            raise SystemExit('{} {} from {}:\n{}'.format(e.code, e.reason, url, body))


def get(url, access_token):
    """GET a resource with a bearer token."""
    request = urllib.request.Request(
        url, headers={'Authorization': 'Bearer ' + access_token})
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return json.loads(response.read().decode())
    except urllib.error.HTTPError as e:
        return {'_http_error': '{} {}'.format(e.code, e.reason),
                '_body': e.read().decode(errors='replace')[:500]}
    except ValueError as e:
        return {'_error': 'response was not JSON: {}'.format(e)}


def report_mfa_claims(where, claims):
    """Say whether an MFA-bearing claim is present in this source."""
    if claims is None:
        print('  {}: unavailable'.format(where))
        return
    found = {k: claims[k] for k in ('acr', 'amr', 'auth_time') if k in claims}
    if found:
        print('  {}: {}'.format(where, json.dumps(found, sort_keys=True)))
    else:
        print('  {}: no acr / amr / auth_time'.format(where))


def decode_segment(segment):
    segment += '=' * (-len(segment) % 4)
    return json.loads(base64.urlsafe_b64decode(segment.encode()).decode())


def show_jwt(name, token):
    print('\n--- {} ---'.format(name))
    parts = token.split('.')
    if len(parts) != 3:
        print('  not a compact JWT ({} segments); raw value:'.format(len(parts)))
        print('  ' + token)
        return None
    try:
        payload = decode_segment(parts[1])
    except (ValueError, UnicodeDecodeError) as e:
        print('  could not decode payload: {}'.format(e))
        return None
    print(json.dumps(payload, indent=2, sort_keys=True))
    return payload


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('config', help='path to the module config.json')
    parser.add_argument('--scope', help='override the configured scope')
    parser.add_argument('--introspection-endpoint',
                        help='also introspect the access token at this endpoint')
    args = parser.parse_args()

    with open(args.config) as f:
        config = json.load(f)

    oauth = config['oauth']
    client_id = oauth['client']['id']
    client_secret = oauth['client']['secret']
    scope = args.scope or oauth['scope']
    basic_auth = config.get('http_basic_auth', True)

    if 'openid' not in scope.split():
        print('WARNING: "openid" is not in the scope, so there will be no ID '
              'token and no acr claim.\n', file=sys.stderr)

    print('client:   {}'.format(client_id))
    print('scope:    {}'.format(scope))
    print('endpoint: {}'.format(oauth['device_endpoint']))

    auth = post(oauth['device_endpoint'], {'scope': scope},
                client_id, client_secret, basic_auth)
    if 'device_code' not in auth:
        raise SystemExit('device authorization failed:\n'
                         + json.dumps(auth, indent=2))

    print('\nVisit:\n\n    {}\n'.format(
        auth.get('verification_uri_complete') or auth.get('verification_uri')))
    if not auth.get('verification_uri_complete'):
        print('and enter the code: {}\n'.format(auth.get('user_code')))
    print('Waiting for you to finish logging in...')

    interval = int(auth.get('interval', 5))
    deadline = time.time() + int(auth.get('expires_in', 300))
    token = None
    while time.time() < deadline:
        time.sleep(interval)
        response = post(oauth['token_endpoint'],
                        {'grant_type': DEVICE_CODE_GRANT,
                         'device_code': auth['device_code']},
                        client_id, client_secret, basic_auth)
        error = response.get('error')
        if not error:
            token = response
            break
        if error == 'authorization_pending':
            continue
        if error == 'slow_down':
            interval += 1
            continue
        raise SystemExit('token endpoint returned: {}'.format(
            json.dumps(response, indent=2)))

    if token is None:
        raise SystemExit('timed out waiting for authorization')

    print('\n=== token endpoint response (keys) ===')
    print(', '.join(sorted(token)))

    id_token = token.get('id_token')
    claims = None
    if id_token:
        claims = show_jwt('id_token claims', id_token)
    else:
        print('\n--- id_token ---')
        print('  not returned (is "openid" in the scope?).  The module reads '
              'userinfo, so this is not fatal.')

    # Is the access token itself a JWT carrying the same claims?
    access_token = token['access_token']
    access_claims = None
    if access_token.count('.') == 2:
        access_claims = show_jwt('access_token claims (it is a JWT)', access_token)
    else:
        print('\n--- access_token ---')
        print('  opaque, not a JWT ({} chars, {} dots)'.format(
            len(access_token), access_token.count('.')))

    # What does the userinfo endpoint say?
    userinfo = None
    if 'userinfo_endpoint' in oauth:
        print('\n--- userinfo endpoint ---')
        userinfo = get(oauth['userinfo_endpoint'], access_token)
        if '_http_error' in userinfo or '_error' in userinfo:
            print('  {}'.format(userinfo.get('_http_error') or userinfo['_error']))
            userinfo = None
        else:
            print('  claims returned: {}'.format(', '.join(sorted(userinfo))))

    # And token introspection, if the provider offers it?
    introspection = None
    endpoint = args.introspection_endpoint
    if endpoint:
        print('\n--- introspection endpoint ---')
        introspection = post(endpoint, {'token': access_token},
                             client_id, client_secret, basic_auth)
        if introspection.get('error'):
            print('  {}'.format(json.dumps(introspection)))
            introspection = None
        else:
            print('  claims returned: {}'.format(', '.join(sorted(introspection))))

    print('\n=== where do the MFA claims actually live? ===')
    report_mfa_claims('id_token      ', claims)
    report_mfa_claims('access_token  ', access_claims)
    report_mfa_claims('userinfo      ', userinfo)
    report_mfa_claims('introspection ', introspection)

    print('\n=== what this means for the mfa config ===')
    acr = (claims or {}).get('acr')
    amr = (claims or {}).get('amr')
    if acr is None and amr is None:
        print('Neither acr nor amr is present.  This IdP records nothing about '
              'MFA, so "if_absent" would apply to every login through it.')
    if acr is not None:
        print('acr = {!r}'.format(acr))
        print('  -> put this in mfa.acr_values if it means MFA was done.')
    if amr is not None:
        print('amr = {!r}'.format(amr))
        print('  -> put the relevant entries in mfa.amr_values.')
    print('\nRun this again with an IdP that does NOT do MFA to confirm the '
          'claim disappears; a claim that is always present enforces nothing.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
