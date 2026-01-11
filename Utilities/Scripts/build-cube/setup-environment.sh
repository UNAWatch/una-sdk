#!/bin/bash

set -e

echo "Setting up STM32CubeIDE environment..."

export DISPLAY=
apt update && apt install -y python3 python3-bincopy python3-git python3-pil
mkdir -p /tmp/bin && ln -s /usr/bin/python3 /tmp/bin/python && export PATH=/tmp/bin:$PATH
export PATH=/tmp/bin:/opt/st/stm32cubeide_*/:`find /opt/st/stm32cubeide_* -name arm-none-eabi-objcopy | head -n 1 | xargs dirname`:$PATH
# Ensure python is available for post-build scripts
export PYTHONPATH=/usr/lib/python3/dist-packages:$PYTHONPATH

git config --global --add safe.directory $GITHUB_WORKSPACE

STM32CUBEIDE_DIR="$(ls -1d /opt/st/stm32cubeide_* 2>/dev/null | sort -V | tail -n 1)"
if [ -z "$STM32CUBEIDE_DIR" ] || [ ! -x "$STM32CUBEIDE_DIR/stm32cubeide" ]; then
  echo "ERROR: STM32CubeIDE not found under /opt/st (expected /opt/st/stm32cubeide_*/stm32cubeide)" >&2
  ls -la /opt/st || true
  exit 1
fi

OBJCOPY_BIN="`find "$STM32CUBEIDE_DIR" -name arm-none-eabi-objcopy -type f | head -n 1`"
if [ -z "$OBJCOPY_BIN" ] || [ ! -x "$OBJCOPY_BIN" ]; then
  echo "ERROR: arm-none-eabi-objcopy not found under $STM32CUBEIDE_DIR" >&2
  find "$STM32CUBEIDE_DIR" -maxdepth 4 -type f -name arm-none-eabi-objcopy -print || true
  exit 1
fi

echo "Using STM32CubeIDE: $STM32CUBEIDE_DIR/stm32cubeide"
echo "Using OBJCOPY: $OBJCOPY_BIN"
# Set build version
BUILD_VERSION="0.0.0-dev" # When version not recognized
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


echo "Environment setup complete."