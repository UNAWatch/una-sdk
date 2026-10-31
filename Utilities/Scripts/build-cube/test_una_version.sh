#!/bin/bash
# A copied example (Docs/sdk-setup.md) is not a git repo. una-version.sh used
# to treat that failure as dubious ownership and store the rev-parse error
# text in the global safe.directory. This runs the script against an empty
# directory with an isolated GIT_CONFIG_GLOBAL.
set -eu

unset BUILD_VERSION

script_dir=$(cd "$(dirname "$0")" && pwd)
empty=$(mktemp -d)
cfg=$(mktemp)
export GIT_CONFIG_GLOBAL="$cfg"

cleanup() {
  rm -rf "$empty"
  rm -f "$cfg"
}
trap cleanup EXIT

# Outside the checkout: rev-parse walks parent directories, and a directory
# inside this repo is a real work tree.
case "$empty" in
  "$script_dir"*) echo "temp dir is inside the script tree: $empty" >&2; exit 1 ;;
esac

out=$(bash "$script_dir/una-version.sh" "$empty")

status=0
if git config --global --get-regexp '^safe\.' >/dev/null 2>&1; then
  echo "safe.directory was written outside a repository:" >&2
  git config --global --get-regexp '^safe\.' >&2
  status=1
fi

case "$out" in
  *BUILD_VERSION=0.0.0-dev*) ;;
  *) echo "expected BUILD_VERSION=0.0.0-dev, got: $out" >&2; status=1 ;;
esac

exit "$status"
