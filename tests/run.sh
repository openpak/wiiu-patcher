#!/bin/sh
# Host test of the backup and rollback rules (app/source/Backup.cpp): ./tests/run.sh
set -e
cd "$(dirname "$0")/.."
root=$(mktemp -d "${TMPDIR:-.}/backup-test.XXXXXX")
trap 'rm -rf "$root"' EXIT
g++ -std=c++20 -Wall -Iapp/source tests/backup_test.cpp app/source/Backup.cpp -o "$root/t"
"$root/t" "$root"
