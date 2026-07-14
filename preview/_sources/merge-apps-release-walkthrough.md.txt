# Post-merge tag push walkthrough

Use this after merging `import/merge-apps` into `main` on [una-sdk](https://github.com/UNAWatch/una-sdk).

Apps are imported as a **snapshot** under `Examples/` (no una-apps git history on `main`). Use **Squash merge** or **Rebase and merge** on the PR — a merge commit is not required.

## Phase 1 — Merge (GitHub)

1. Open the PR: `import/merge-apps` → `main`.
2. Confirm CI on the PR is green.
3. Merge into `main` (squash or rebase-and-merge is fine).
4. Locally:
   ```bash
   cd una-sdk
   git checkout main
   git pull origin main
   ```

## Phase 2 — Retag apps release on `main` (CI **on**)

The earlier `apps-v0.1.9-rc1` tag (if pushed) pointed at pre-import history and is not on `main`. Move it to the merge commit:

```bash
git tag -a -f apps-v0.1.9-rc1 -m "Apps release 0.1.9-rc1" HEAD
git push -f origin apps-v0.1.9-rc1
```

`sdk-v0.1.4` can stay as-is (it marks a pre-import SDK release commit that remains in history).

**Expect:** One CI Apps run for the retagged `apps-v*` push, using the new root `.github/workflows/apps-ci.yml`.

## Phase 3 — Kernel (internal)

```bash
cd una-kernel
git checkout main
git pull
cd SDK && git fetch origin && git checkout main && git pull && cd ..
git add SDK
git commit -m "chore(SDK): bump submodule after una-apps merge"
git push
```

## Phase 4 — Archive una-apps

Copy [una-apps-archive-README.md](una-apps-archive-README.md) to the root `README.md` of [una-apps](https://github.com/UNAWatch/una-apps), then archive the repository on GitHub.

## Optional — historical `sdk-v*` tags

Only if you want old SDK release markers on GitHub:

```bash
bash Utilities/Scripts/migrate-release-tags.sh   # creates sdk-v* from origin v* tags
git push origin 'refs/tags/sdk-v*:refs/tags/sdk-v*'
```

Skip bulk `apps-v*` historical tags — those commits are not on `main` after the snapshot import.
