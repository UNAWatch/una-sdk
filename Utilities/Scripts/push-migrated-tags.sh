#!/bin/bash
# Push migrated sdk-v* / apps-v* tags to origin.
# Usage:
#   ./push-migrated-tags.sh latest          # apps-v0.1.9-rc1 + sdk-v0.1.4 only
#   ./push-migrated-tags.sh remaining       # all prefixed tags except those two
#   ./push-migrated-tags.sh all             # every sdk-v* and apps-v*
set -euo pipefail

LATEST_APPS_TAG="${LATEST_APPS_TAG:-apps-v0.1.9-rc1}"
LATEST_SDK_TAG="${LATEST_SDK_TAG:-sdk-v0.1.4}"
REMOTE="${REMOTE:-origin}"
MODE="${1:-}"

if [ -z "$MODE" ]; then
  echo "Usage: $0 {latest|remaining|all}" >&2
  exit 1
fi

cd "$(git rev-parse --show-toplevel)"

push_tags() {
  local tag
  for tag in "$@"; do
    if git rev-parse "$tag" >/dev/null 2>&1; then
      echo "Pushing $tag ..."
      git push "$REMOTE" "$tag"
    else
      echo "SKIP (missing locally): $tag" >&2
    fi
  done
}

mapfile -t ALL_APPS < <(git tag -l 'apps-v*' | sort -V)
mapfile -t ALL_SDK < <(git tag -l 'sdk-v*' | sort -V)

case "$MODE" in
  latest)
    push_tags "$LATEST_APPS_TAG" "$LATEST_SDK_TAG"
    ;;
  remaining)
    for tag in "${ALL_APPS[@]}"; do
      [ "$tag" = "$LATEST_APPS_TAG" ] && continue
      push_tags "$tag"
    done
    for tag in "${ALL_SDK[@]}"; do
      [ "$tag" = "$LATEST_SDK_TAG" ] && continue
      push_tags "$tag"
    done
    ;;
  all)
    push_tags "${ALL_APPS[@]}" "${ALL_SDK[@]}"
    ;;
  *)
    echo "Unknown mode: $MODE" >&2
    exit 1
    ;;
esac

echo "Done ($MODE)."
