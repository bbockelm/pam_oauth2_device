#!/bin/bash
# Runs the given test binaries with the OAuth2 mock server listening on
# localhost:8042, then shuts the server down again.
#
# Usage: ./run_tests.sh test_config test_pam_oauth2_device
set -euo pipefail

cd "$(dirname "$0")"

PORT=8042
PYTHON=${PYTHON:-python3}

if [ $# -eq 0 ]; then
    echo "usage: $0 TEST [TEST...]" >&2
    exit 2
fi

"$PYTHON" ./mock_server.py &
MOCK_PID=$!

cleanup() {
    kill "$MOCK_PID" 2>/dev/null || true
    wait "$MOCK_PID" 2>/dev/null || true
}
trap cleanup EXIT

# Wait for the mock server to accept connections (up to ~10s).
for _ in $(seq 1 100); do
    if ! kill -0 "$MOCK_PID" 2>/dev/null; then
        echo "mock server exited before it started listening" >&2
        exit 1
    fi
    if "$PYTHON" -c "
import socket, sys
s = socket.socket()
s.settimeout(0.5)
sys.exit(0 if s.connect_ex(('127.0.0.1', $PORT)) == 0 else 1)
"; then
        break
    fi
    sleep 0.1
done || true

if ! "$PYTHON" -c "
import socket, sys
s = socket.socket()
s.settimeout(0.5)
sys.exit(0 if s.connect_ex(('127.0.0.1', $PORT)) == 0 else 1)
"; then
    echo "mock server did not start listening on port $PORT" >&2
    exit 1
fi

status=0
for test in "$@"; do
    echo "== ./$test =="
    if ! "./$test"; then
        status=1
    fi
done

# Checks the PAM stacks documented in README.md; skips itself where it cannot
# run (no root, no pamtester).
echo "== ./pam_stack_test.sh =="
if ! ./pam_stack_test.sh; then
    status=1
fi

exit $status
