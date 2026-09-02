# PAM module testing

The tests need the GoogleTest development files (`gtest-devel` on EL,
`libgtest-dev` on Debian/Ubuntu); they are already installed in the dev
container, see [`../.devcontainer`](../.devcontainer).

From the top of the tree:

```
make test
```

That builds the three test binaries and runs them via `run_tests.sh`, which
starts `mock_server.py` on port 8042 for the duration of the run and shuts it
down afterwards.  Equivalently, from this directory:

```
make check
```

To build the binaries without running them, use `make`.  Individual binaries can
be run by hand, but `test_pam_oauth2_device` needs the mock server, so either
start `./mock_server.py` in another terminal first or invoke it through
`./run_tests.sh test_pam_oauth2_device`.

## The test binaries

  * `test_config` - configuration file parsing, against the fixtures in `data/`
    and against `../config_template.json`.
  * `unit` - QR code rendering and the authorization decision logic.
  * `test_pam_oauth2_device` - the OAuth2 device flow against `mock_server.py`.
  * `pam_stack_test.sh` - drives the PAM stacks documented in the top-level
    README with `pamtester` and a stub module, checking that a second factor is
    demanded and honoured and that a `*bypass*` account is not waved through.
    Needs root and `pamtester`; skips itself otherwise, which it does inside the
    dev container, since that runs as an unprivileged user.  Run it there with
    `sudo ./pam_stack_test.sh`.  CI runs it for real on both EL8 and EL9.
