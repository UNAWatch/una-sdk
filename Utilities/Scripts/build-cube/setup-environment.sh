#!/bin/sh


echo "Setting up STM32CubeIDE environment..."

set -e

SCRIPT_DIR="$( cd "$( dirname "$0" )" &amp;&amp; pwd )"
SCRIPT_DIR=${SCRIPT_DIR:-$(dirname "$0")}
# Source the split scripts
. "$SCRIPT_DIR/find-cube.sh"
. "$SCRIPT_DIR/una-version.sh"

echo "Environment setup complete."