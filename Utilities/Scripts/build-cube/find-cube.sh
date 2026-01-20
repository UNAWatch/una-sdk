#!/bin/bash

set -e

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