#!/bin/bash

set -e

# Source version script to get BUILD_VERSION
. "$(dirname "$0")/una-version.sh"

IMPORT_PATH="$1"

if [ -z "$IMPORT_PATH" ]; then
  echo "Usage: $0 <import_path>"
  exit 1
fi

NAME="$(basename $IMPORT_PATH)"
DATA_DIR="$UNA_WORKSPACE/.headless-build/$NAME"
echo "Building '$NAME' in '$DATA_DIR'"

# Patch __BUILD_VERSION__ in the imported project's prefs (assumes project name matches basename of IMPORT_PATH)
PREFS_FILE="$(dirname $IMPORT_PATH)/$NAME/.settings/org.eclipse.cdt.core.prefs"
if [ -f "$PREFS_FILE" ]; then
  if [ -n "$BUILD_VERSION" ]; then
    sed -i 's#/__BUILD_VERSION__/value=.*#/__BUILD_VERSION__/value='"$BUILD_VERSION"'#g' "$PREFS_FILE" \
        && echo "Overridden __BUILD_VERSION__ to '$BUILD_VERSION' in $PREFS_FILE" \
        || echo "Pattern __BUILD_VERSION__ not found in $PREFS_FILE"
  fi
else
  echo "Warning: Prefs file not found at $PREFS_FILE"
fi

# Build with overridden prefs
stm32cubeide --launcher.suppressErrors -nosplash \
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
  -import "$IMPORT_PATH" \
  -data "$DATA_DIR" \
  -build all \
  -vmargs -Djava.awt.headless=true

echo "Build complete for $IMPORT_PATH"