#!/bin/bash

set -e

# Logging function
log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >&2
}

log "Starting una-version.sh script"

# Set build version
if [ -n "$BUILD_VERSION" ]; then
  log "BUILD_VERSION already set: $BUILD_VERSION"
  export BUILD_VERSION
  log "BUILD_VERSION=$BUILD_VERSION"
  log "Exiting early as BUILD_VERSION was already set"
  return 0
fi

BUILD_VERSION="0.0.0-dev" # When version not recognized
log "Default BUILD_VERSION set to: $BUILD_VERSION"

if [ -z "$1" ]; then
  UNA_GIT_DIR=$(pwd)
  log "No argument provided, using current directory as UNA_GIT_DIR: $UNA_GIT_DIR"
else
  UNA_GIT_DIR="$1"
  log "Argument provided, UNA_GIT_DIR set to: $UNA_GIT_DIR"
fi

log "Workspace: $UNA_GIT_DIR"
if cd "$UNA_GIT_DIR"; then
  log "Successfully changed directory to: $UNA_GIT_DIR"
else
  log "ERROR: Failed to cd to $UNA_GIT_DIR"
  exit 1
fi

# Get top-level git dir for safe.directory (handles nested submodules)
# Safe bootstrap for repo toplevel (Docker dubious ownership)
toplevel=""
log "Attempting to get git directory"
git_dir=$(git rev-parse --git-dir 2>&1)
log "git rev-parse --git-dir output: '$git_dir'"
if echo "$git_dir" | grep -q "dubious ownership\\|unsafe repository\\|fatal:" ; then
  log "Detected dubious ownership or unsafe repository, configuring safe.directory"
  git config --global safe.directory '*'
  log "Set global safe.directory to '*' "
  toplevel=$(git rev-parse --show-toplevel 2>&1)
  log "git rev-parse --show-toplevel output after config: '$toplevel'"
  git config --global safe.directory "$toplevel"
  log "Set global safe.directory to '$toplevel'"
else
  toplevel=$(git rev-parse --show-toplevel 2>&1)
  log "git rev-parse --show-toplevel output: '$toplevel'"
fi
log "toplevel: $toplevel"
if cd "$toplevel"; then
  log "Successfully changed directory to toplevel: $toplevel"
else
  log "ERROR: Failed to cd to $toplevel"
  exit 1
fi
UNA_WORKSPACE=$toplevel
log "UNA_WORKSPACE set to: $UNA_WORKSPACE"

if git rev-parse --git-dir > /dev/null 2>&1; then
  log "Git directory found, proceeding with version detection"
  COMMIT_HASH=$(git rev-parse --short=7 HEAD 2>&1)
  log "COMMIT_HASH: $COMMIT_HASH"
  if git describe --exact-match --tags HEAD >/dev/null 2>&1; then
    log "On exact tag"
    TAG=$(git describe --exact-match --tags HEAD 2>&1)
    log "TAG: $TAG"
    TAG=${TAG#v}
    log "TAG after removing 'v' prefix: $TAG"
    BUILD_VERSION=$TAG
    log "BUILD_VERSION set to TAG: $BUILD_VERSION"
  else
    log "Not on exact tag"
    DESC=$(git describe --always --tags --abbrev=7 2>&1)
    log "DESC: $DESC"
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
        log "TAG before removing 'v': $TAG"
        TAG=${TAG#v}
        log "TAG after removing 'v': $TAG"
        log "HASH: $HASH"
        BUILD_VERSION="${TAG}-${HASH}"
        log "BUILD_VERSION set to TAG-HASH: $BUILD_VERSION"
        ;;
    esac
  fi
  log "Checking for uncommitted changes"
  git_status_output=$(git status --porcelain 2>&1)
  log "git status --porcelain output: '$git_status_output'"
  if [ -n "$git_status_output" ]; then
    log "Uncommitted changes detected, appending '-dirty'"
    BUILD_VERSION="${BUILD_VERSION}-dirty"
  else
    log "No uncommitted changes"
  fi
else
  log "No git directory found, using default version"
fi

export BUILD_VERSION
log "Final BUILD_VERSION=$BUILD_VERSION"
log "Script completed successfully"