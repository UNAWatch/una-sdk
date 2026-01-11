#!/bin/bash

set -e

IMPORT_PATH="$1"

if [ -z "$IMPORT_PATH" ]; then
  echo "Usage: $0 <import_path>"
  exit 1
fi

echo "Building $IMPORT_PATH..."

# Import project first
stm32cubeide --launcher.suppressErrors -nosplash \
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
  -data "${DATA_DIR:-/workspace/.vscode/workspace}" \
  -import "$IMPORT_PATH"

# Patch __BUILD_VERSION__ in the imported project's prefs (assumes project name matches basename of IMPORT_PATH)
WORKSPACE="${DATA_DIR:-/workspace/.vscode/workspace}"
PREFS_FILE="$IMPORT_PATH/.settings/org.eclipse.cdt.core.prefs"

if [ -f "$PREFS_FILE" ]; then
  sed -i 's#/__BUILD_VERSION__/value=.*#/__BUILD_VERSION__/value='"$BUILD_VERSION"'#g' "$PREFS_FILE" || echo "Pattern __BUILD_VERSION__ not found in $PREFS_FILE"
  echo "Overridden __BUILD_VERSION__ to '$BUILD_VERSION' in $PREFS_FILE"
else
  echo "Warning: Prefs file not found at $PREFS_FILE"
fi

# Build with overridden prefs
stm32cubeide --launcher.suppressErrors -nosplash \
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
  -data "$WORKSPACE" \
  -build all \
  -vmargs -Djava.awt.headless=true

echo "Build complete for $IMPORT_PATH"