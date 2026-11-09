#!/bin/bash
# A copied example (Docs/sdk-setup.md) is not a git repo. una-version.sh used
# to treat that failure as dubious ownership and store the rev-parse error
# text in the global safe.directory. This runs the script against an empty
# directory with an isolated GIT_CONFIG_GLOBAL.
set -eu

unset BUILD_VERSION

script_dir=$(cd "$(dirname "$0")" && pwd)
empty=$(mktemp -d)
inside=""
cfg=$(mktemp)
export GIT_CONFIG_GLOBAL="$cfg"

cleanup() {
  rm -rf "$empty" ${inside:+"$inside"} ${owned:+"$owned"} ${fake:+"$fake"}
  rm -f "$cfg" ${log:+"$log"}
}
trap cleanup EXIT

# Git walks parent directories looking for .git. The ceiling is a directory
# it must not enter, so it has to be the temp directory's parent. Listing the
# temp directory itself does not stop the walk. This is what happens when
# TMPDIR sits inside a checkout.
out=$(GIT_CEILING_DIRECTORIES="$(dirname "$empty")" bash "$script_dir/una-version.sh" "$empty")

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

# Same check with the empty directory created inside this checkout. Without
# the ceiling, rev-parse finds the SDK repo and the assertion above is a
# false failure.
inside=$(mktemp -d "$script_dir/.una-version-empty.XXXXXX")
out=$(GIT_CEILING_DIRECTORIES="$(dirname "$inside")" bash "$script_dir/una-version.sh" "$inside")
case "$out" in
  *BUILD_VERSION=0.0.0-dev*) ;;
  *) echo "directory inside the checkout was treated as the SDK repo: $out" >&2; status=1 ;;
esac

# Ownership failure used to run `git config --global safe.directory '*'`,
# and later a one-shot `git -c safe.directory=*`. Either one trusts the
# checkout, so a later git status runs its core.fsmonitor hook or an alias.
# A fake git reports dubious ownership. The script must not pass -c and must
# not invoke `git config`. The version stays the default.
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
# Ownership errors are translated. This one is not the English phrase.
# Only a -c safe.directory invocation may succeed.
if [ "$1" != "-c" ]; then
  echo "fatal: détection d'un propriétaire douteux" >&2
  exit 128
fi
case " $* " in
  *" --show-toplevel "*) echo "$OWNED_TOP"; exit 0 ;;
  *" describe "*) echo "sdk-v1.2.3"; exit 0 ;;
  *" status "*) exit 0 ;;
  *" rev-parse "*) echo "abc1234"; exit 0 ;;
esac
exit 1
EOF
chmod +x "$fake/git"

cleanup() {
  rm -rf "$empty" "$inside" "$owned" "$fake"
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
if grep -q -- "-c" "$log" || grep -q "safe.directory" "$log"; then
  echo "dubious-ownership path overrode safe.directory:" >&2
  cat "$log" >&2
  status=1
fi
if git config --global --get-regexp '^safe\.' >/dev/null 2>&1; then
  echo "safe.directory was written for a dubious repository:" >&2
  git config --global --get-regexp '^safe\.' >&2
  status=1
fi
case "$out" in
  *BUILD_VERSION=0.0.0-dev*) ;;
  *) echo "dubious-ownership path should keep the default version, got: $out" >&2; status=1 ;;
esac

# setup-environment.sh and build-cube.sh source this script. Sourcing must
# not leave GIT_CONFIG_* behind, including when the caller already set some.
sourced=$(
  set +eu
  export GIT_CONFIG_COUNT=2
  export GIT_CONFIG_KEY_0=user.name
  export GIT_CONFIG_VALUE_0=kept
  export CALL_LOG="$log" OWNED_TOP="$owned"
  export PATH="$fake:$PATH"
  unset BUILD_VERSION
  # shellcheck disable=SC1090
  . "$script_dir/una-version.sh" "$owned" >/dev/null
  printf 'COUNT=%s\nKEY=%s\nVALUE=%s\nBUILD=%s\n' \
    "${GIT_CONFIG_COUNT-}" "${GIT_CONFIG_KEY_0-}" "${GIT_CONFIG_VALUE_0-}" "${BUILD_VERSION-}"
  if type _una_git >/dev/null 2>&1; then
    echo HELPER=set
  else
    echo HELPER=unset
  fi
)
printf '%s\n' "$sourced" | grep -q '^COUNT=2$' || { echo "sourced script replaced GIT_CONFIG_COUNT: $sourced" >&2; status=1; }
printf '%s\n' "$sourced" | grep -q '^VALUE=kept$' || { echo "sourced script replaced GIT_CONFIG_VALUE_0: $sourced" >&2; status=1; }
printf '%s\n' "$sourced" | grep -q '^BUILD=0.0.0-dev$' || { echo "sourced script trusted a rejected checkout: $sourced" >&2; status=1; }
printf '%s\n' "$sourced" | grep -q '^HELPER=unset$' || { echo "sourced script left _una_git defined: $sourced" >&2; status=1; }

exit "$status"
