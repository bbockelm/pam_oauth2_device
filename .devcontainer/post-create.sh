#!/bin/bash
# Runs once after the dev container is created.
set -euo pipefail

echo "Toolchain: $(g++ --version | head -1)"
echo "GoogleTest: $(rpm -q gtest-devel)"

cat <<'MSG'

Ready.  Common tasks:

  make            build pam_oauth2_device.so
  make test       build and run the unit tests (starts the mock server)
  make clean

MSG
