#!/bin/bash
set -e

DOCS_DIR="$(cd "$(dirname "$0")/../../Docs" && pwd)"

echo "Building Una-Watch SDK documentation..."

cd "$DOCS_DIR"

# Install dependencies if not present
pip install --user -r requirements.txt

# Build HTML Docs
make clean
make html

echo "Documentation built successfully in $DOCS_DIR/_build/html"

# Optional: Copy to a deploy directory
mkdir -p ../../Docs/html
cp -r _build/html/* ../../Docs/html/

echo "HTML Docs copied to ../../Docs/html"