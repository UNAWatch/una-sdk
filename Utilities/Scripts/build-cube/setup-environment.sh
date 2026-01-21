#!/bin/bash

set -e

echo "Setting up STM32CubeIDE environment..."

export PATH=$PATH:$UNA_SDK/Utilities/Scripts/build-cube

# Source the split scripts
. find-cube.sh
. una-version.sh

echo "Environment setup complete."