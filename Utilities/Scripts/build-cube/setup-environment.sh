#!/bin/bash


echo "Setting up STM32CubeIDE environment..."

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# Source the split scripts
. "$SCRIPT_DIR/find-cube.sh"
. "$SCRIPT_DIR/una-version.sh"

echo "Environment setup complete."