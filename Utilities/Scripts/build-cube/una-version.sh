#!/bin/bash

# Logging function
log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >&2
}

log "Starting una-version.sh script"

# Check if git is available
if ! command -v git >/dev/null 2>&1; then
  log "Git command not found, using default version"
  BUILD_VERSION="0.0.0-dev"
  export BUILD_VERSION
  echo "BUILD_VERSION=$BUILD_VERSION"
  log "Script completed with default version due to no git"
  exit 0
fi

# Set build version
if [ -n "$BUILD_VERSION" ]; then
  log "BUILD_VERSION already set: $BUILD_VERSION"
  export BUILD_VERSION
  log "BUILD_VERSION=$BUILD_VERSION"
  log "Exiting early as BUILD_VERSION was already set"
  echo "BUILD_VERSION=$BUILD_VERSION"
  # Sourced, return keeps the caller's shell; run with bash, exit ends it.
  return 0 2>/dev/null || exit 0
fi

BUILD_VERSION="0.0.0-dev" # When version not recognized
log "Default BUILD_VERSION set to: $BUILD_VERSION"

# Tag family prefix: apps- / sdk- (see UNA_VERSION_TAG_PREFIX or 2nd argument)
UNA_VERSION_TAG_PREFIX="${UNA_VERSION_TAG_PREFIX:-}"

if [ -z "$1" ]; then
  UNA_GIT_DIR=$(pwd)
  log "No argument provided, using current directory as UNA_GIT_DIR: $UNA_GIT_DIR"
else
  UNA_GIT_DIR="$1"
  log "Argument provided, UNA_GIT_DIR set to: $UNA_GIT_DIR"
  if [ -n "$2" ] && [ -z "$UNA_VERSION_TAG_PREFIX" ]; then
    UNA_VERSION_TAG_PREFIX="$2"
    log "Tag prefix from argument: $UNA_VERSION_TAG_PREFIX"
  fi
fi

log "Workspace: $UNA_GIT_DIR"
if cd "$UNA_GIT_DIR"; then
  log "Successfully changed directory to: $UNA_GIT_DIR"
else
  log "ERROR: Failed to cd to $UNA_GIT_DIR"
  exit 1
fi

# Discover the toplevel without writing git config and without reading the
# error text. Git translates "dubious ownership", so matching that English
# phrase misses it and the build reports 0.0.0-dev. A plain rev-parse fails
# for "not a repository" and for ownership. The one-shot -c succeeds only for
# ownership. Later git commands in this script repeat that -c.
# build-cube.sh and setup-environment.sh source this file, so the override is
# not exported, and it is unset before we return.
_una_safe_dir=""
_una_git() {
  if [ -n "${_una_safe_dir:-}" ]; then
    git -c "safe.directory=${_una_safe_dir}" "$@"
  else
    git "$@"
  fi
}
_una_drop_git_override() {
  unset _una_safe_dir
  unset -f _una_git _una_drop_git_override
}

log "Attempting to get git toplevel"
toplevel=$(git rev-parse --show-toplevel 2>/dev/null || true)
if [ -z "$toplevel" ]; then
  toplevel=$(git -c safe.directory='*' rev-parse --show-toplevel 2>/dev/null || true)
  if [ -n "$toplevel" ]; then
    log "Plain rev-parse failed; safe.directory retry found: '$toplevel'"
    _una_safe_dir=$toplevel
  else
    toplevel="fatal: git rev-parse failed"
  fi
else
  log "git rev-parse --show-toplevel output: '$toplevel'"
fi
log "toplevel: $toplevel"
if echo "$toplevel" | grep -q "fatal:"; then
  log "Not in a git repository, using default version"
  export BUILD_VERSION
  echo "BUILD_VERSION=$BUILD_VERSION"
  log "Script completed with default version"
  _una_drop_git_override
  exit 0
fi
if cd "$toplevel"; then
  log "Successfully changed directory to toplevel: $toplevel"
else
  log "ERROR: Failed to cd to $toplevel"
  _una_drop_git_override
  exit 1
fi
UNA_WORKSPACE=$toplevel
log "UNA_WORKSPACE set to: $UNA_WORKSPACE"

# Auto-detect tag prefix for merged una-sdk (apps under Examples/, SDK at repo root)
if [ -z "$UNA_VERSION_TAG_PREFIX" ]; then
  case "$UNA_GIT_DIR" in
    *"/Examples/"*|*"/Examples"|*"\\Examples\\"*|*"\\Examples")
      UNA_VERSION_TAG_PREFIX="apps-"
      ;;
  esac
  if [ -z "$UNA_VERSION_TAG_PREFIX" ] && [ -f "$toplevel/cmake/una-sdk.cmake" ]; then
    UNA_VERSION_TAG_PREFIX="sdk-"
  fi
  if [ -n "$UNA_VERSION_TAG_PREFIX" ]; then
    log "Auto-detected tag prefix: $UNA_VERSION_TAG_PREFIX"
  fi
fi

# Strip release tag to BUILD_VERSION (apps-v1.2.3 -> 1.2.3, v1.2.3 -> 1.2.3)
normalize_tag_version() {
  local tag="$1"
  case "$tag" in
    apps-v*) echo "${tag#apps-v}" ;;
    sdk-v*) echo "${tag#sdk-v}" ;;
    v*) echo "${tag#v}" ;;
    *) echo "$tag" ;;
  esac
}

git_describe_exact() {
  if [ -n "$UNA_VERSION_TAG_PREFIX" ]; then
    _una_git describe --exact-match --tags --match "${UNA_VERSION_TAG_PREFIX}v*" HEAD 2>/dev/null
  else
    _una_git describe --exact-match --tags HEAD 2>/dev/null
  fi
}

git_describe_nearest() {
  if [ -n "$UNA_VERSION_TAG_PREFIX" ]; then
    _una_git describe --always --tags --abbrev=7 --match "${UNA_VERSION_TAG_PREFIX}v*" 2>/dev/null
  else
    _una_git describe --always --tags --abbrev=7 2>/dev/null
  fi
}

if _una_git rev-parse --git-dir > /dev/null 2>&1; then
  log "Git directory found, proceeding with version detection"
  if [ -n "$UNA_VERSION_TAG_PREFIX" ]; then
    log "Using tag match pattern: ${UNA_VERSION_TAG_PREFIX}v*"
  fi
  COMMIT_HASH=$(_una_git rev-parse --short=7 HEAD 2>&1 || echo "failed")
  log "COMMIT_HASH: $COMMIT_HASH"
  if git_describe_exact >/dev/null 2>&1; then
    log "On exact tag"
    TAG=$(git_describe_exact 2>&1 || echo "failed")
    log "TAG: $TAG"
    if [ "$TAG" != "failed" ]; then
      BUILD_VERSION=$(normalize_tag_version "$TAG")
      log "BUILD_VERSION set to TAG: $BUILD_VERSION"
    fi
  else
    log "Not on exact tag"
    DESC=$(git_describe_nearest 2>&1 || echo "failed")
    log "DESC: $DESC"
    if [ "$DESC" != "failed" ]; then
      case "$DESC" in
      g*)
        log "No tags found, DESC starts with 'g'"
        # no tags, use hash
        HASH=${DESC#g}
        log "HASH extracted: $HASH"
        BUILD_VERSION=$HASH
        log "BUILD_VERSION set to HASH: $BUILD_VERSION"
        ;;
      *)
        log "Tags found, parsing TAG and HASH"
        # has tag-hash
        TAG=$(echo $DESC | sed 's/-g.*//')
        HASH=$(echo $DESC | sed 's/.*-g//')
        log "TAG before normalize: $TAG"
        TAG=$(normalize_tag_version "$TAG")
        log "TAG after normalize: $TAG"
        log "HASH: $HASH"
        BUILD_VERSION="${TAG}-${HASH}"
        log "BUILD_VERSION set to TAG-HASH: $BUILD_VERSION"
        ;;
    esac
    fi
    log "Checking for uncommitted changes"
    git_status_output=$(_una_git status --porcelain 2>&1 || echo "failed")
    log "git status --porcelain output: '$git_status_output'"
    if [ "$git_status_output" != "failed" ] && [ -n "$git_status_output" ]; then
      log "Uncommitted changes detected, appending '-dirty'"
      BUILD_VERSION="${BUILD_VERSION}-dirty"
    else
      log "No uncommitted changes"
    fi
  fi
else
  log "No git directory found, using default version"
fi

# app_merging.py requires A.B.C at the start of -appver
if [ "$UNA_VERSION_TAG_PREFIX" = "apps-" ] && ! echo "$BUILD_VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+'; then
  log "Apps BUILD_VERSION '$BUILD_VERSION' is not semver; using 0.0.0-dev"
  BUILD_VERSION="0.0.0-dev"
fi

export BUILD_VERSION
log "Final BUILD_VERSION=$BUILD_VERSION"
log "Script completed successfully"
echo "BUILD_VERSION=$BUILD_VERSION"
_una_drop_git_override
