#!/bin/sh
# Host test of the backup and rollback rules (app/source/Backup.cpp): ./tests/run.sh
set -e
cd "$(dirname "$0")/.."
root=$(mktemp -d "${TMPDIR:-.}/backup-test.XXXXXX")
trap 'rm -rf "$root"' EXIT
g++ -std=c++20 -Wall -Iapp/source tests/backup_test.cpp app/source/Backup.cpp -o "$root/t"
"$root/t" "$root"
# The signed redirect list: real ceiling v18 from openpak.org (signature, tampering, matching, never-set).
g++ -std=c++20 -Wall -Icommon tests/redirect_test.cpp common/redirect_list.cpp common/ed25519.cpp -o "$root/r"
"$root/r" tests/fixtures/ceiling-v18.json
