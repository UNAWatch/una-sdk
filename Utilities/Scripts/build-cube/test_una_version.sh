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

# Ownership failure used to run `git config --global safe.directory '*'`.
# That disable sticks for every repo if the process dies before the follow-up
# write, and it is the wrong place for a version script to grant trust.
# A fake git reports dubious ownership; the script may pass -c, and must not
# invoke `git config`.
owned=$(mktemp -d)
fake=$(mktemp -d)
log=$(mktemp)
cat > "$fake/git" << 'EOF'
#!/bin/bash
printf '%s\n' "$*" >> "$CALL_LOG"
if [ "$1" = "config" ]; then
  echo "git config was invoked: $*" >> "$CALL_LOG"
  exit 0
fi
if [ "$1" = "rev-parse" ] && [ "$2" = "--git-dir" ]; then
  if [ "${GIT_CONFIG_VALUE_0:-}" = "$OWNED_TOP" ]; then
    echo ".git"
    exit 0
  fi
  echo "fatal: detected dubious ownership in repository at '$PWD'"
  exit 128
fi
if [ "$1" = "-c" ]; then
  echo "$OWNED_TOP"
  exit 0
fi
if [ "$1" = "describe" ]; then
  echo "sdk-v1.2.3"
  exit 0
fi
if [ "$1" = "rev-parse" ]; then
  echo "abc1234"
  exit 0
fi
if [ "$1" = "status" ]; then
  exit 0
fi
exit 1
EOF
chmod +x "$fake/git"

cleanup() {
  rm -rf "$empty" "$owned" "$fake"
  rm -f "$cfg" "$log"
}
trap cleanup EXIT

out=$(
  unset GIT_CONFIG_COUNT GIT_CONFIG_KEY_0 GIT_CONFIG_VALUE_0
  export CALL_LOG="$log" OWNED_TOP="$owned"
  export PATH="$fake:$PATH"
  bash "$script_dir/una-version.sh" "$owned"
) || true

if grep -q "git config was invoked" "$log"; then
  echo "dubious-ownership path wrote git config:" >&2
  cat "$log" >&2
  status=1
fi
if git config --global --get-regexp '^safe\.' >/dev/null 2>&1; then
  echo "safe.directory was written for a dubious repository:" >&2
  git config --global --get-regexp '^safe\.' >&2
  status=1
fi
case "$out" in
  *BUILD_VERSION=1.2.3*) ;;
  *) echo "dubious-ownership path should version from the one-shot override, got: $out" >&2; status=1 ;;
esac

exit "$status"
