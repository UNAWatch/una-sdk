#!/bin/bash

set -e

echo "Setting up STM32CubeIDE environment..."

# Source the split scripts
. "$(dirname "$0")/find-cube.sh"
. "$(dirname "$0")/una-version.sh"

echo "Environment setup complete."