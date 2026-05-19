#!/bin/bash
# One-time migration: create sdk-v* prefixed tags in una-sdk.
# For snapshot imports (no apps history on main), skip apps-v* migration — retag
# apps releases on main HEAD instead (see Docs/merge-apps-release-walkthrough.md).
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

echo "Fetching origin tags..."
git fetch origin --tags 2>/dev/null || true

echo "Creating sdk-v* from origin una-sdk tags..."
for tag in $(git ls-remote --tags origin | awk -F/ '{print $NF}' | grep -v '\^{}' | grep '^v' | sort -u); do
  commit=$(git rev-parse "refs/tags/${tag}^{commit}" 2>/dev/null || continue)
  new_tag="sdk-${tag}"
  if git rev-parse "$new_tag" >/dev/null 2>&1; then
    continue
  fi
  git tag -a "$new_tag" -m "SDK release ${tag} (migrated)" "$commit"
  echo "  $new_tag"
done

APPS_GIT="${APPS_GIT:-}"
if [ -z "$APPS_GIT" ] && git remote get-url apps-import >/dev/null 2>&1; then
  APPS_GIT="../.merge-tmp/una-apps.git"
fi

if [ -n "${MIGRATE_APPS_TAGS:-}" ] && { [ -d "$APPS_GIT" ] || [ -f "$APPS_GIT/HEAD" ]; }; then
  echo "Creating apps-v* from filtered una-apps tags..."
  for tag in $(git -C "$APPS_GIT" tag -l 'v*' | sort -V); do
    commit=$(git -C "$APPS_GIT" rev-parse "${tag}^{commit}" 2>/dev/null || continue)
    if ! git cat-file -e "${commit}^{commit}" 2>/dev/null; then
      echo "  SKIP $tag (commit not in una-sdk)" >&2
      continue
    fi
    new_tag="apps-${tag}"
    if git rev-parse "$new_tag" >/dev/null 2>&1; then
      continue
    fi
    git tag -a "$new_tag" -m "Apps release ${tag} (migrated from una-apps)" "$commit"
    echo "  $new_tag"
  done
else
  echo "Skipping apps-v* migration (snapshot import). Set MIGRATE_APPS_TAGS=1 and APPS_GIT to migrate."
fi

echo "Done: $(git tag -l 'sdk-v*' | wc -l) sdk-v* tags, $(git tag -l 'apps-v*' | wc -l) apps-v* tags"
