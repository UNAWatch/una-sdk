#!/bin/bash

set -e

# Set build version
BUILD_VERSION="0.0.0-dev" # When version not recognized
if $GITHUB_WORKSPACE; then
  GITHUB_WORKSPACE=$(pwd)
fi
echo "Workspace: $GITHUB_WORKSPACE"
cd "$GITHUB_WORKSPACE" || echo "Failed to cd to $GITHUB_WORKSPACE"
if git rev-parse --git-dir > /dev/null 2>&1; then
  COMMIT_HASH=$(git rev-parse --short=7 HEAD)
  echo "COMMIT_HASH: $COMMIT_HASH"
  if git describe --exact-match --tags HEAD >/dev/null 2>&1; then
    echo "On exact tag"
    TAG=$(git describe --exact-match --tags HEAD)
    echo "TAG: $TAG"
    TAG=${TAG#v}
    BUILD_VERSION=$TAG
  else
    echo "Not on exact tag"
    DESC=$(git describe --always --tags --abbrev=7)
    echo "DESC: $DESC"
    case "$DESC" in
      g*)
        # no tags, use hash
        HASH=${DESC#g}
        BUILD_VERSION=$HASH
        ;;
      *)
        # has tag-hash
        TAG=$(echo $DESC | sed 's/-g.*//')
        HASH=$(echo $DESC | sed 's/.*-g//')
        TAG=${TAG#v}
        BUILD_VERSION="${TAG}-${HASH}"
        ;;
    esac
  fi
  if [ -n "$(git status --porcelain)" ]; then
    BUILD_VERSION="${BUILD_VERSION}-dirty"
  fi
else
  echo "No git dir"
fi

export BUILD_VERSION
echo "BUILD_VERSION=$BUILD_VERSION"