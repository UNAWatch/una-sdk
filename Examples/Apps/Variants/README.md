# Shipped activity variants

A **variant** is a code-less alias `.uapp` that makes an existing app binary appear in the
launcher as a separate activity (design: una-kernel `Docs/Multi-Activity-Apps-Design.md`).
Each subdirectory here is the source of truth for one **shipped** variant:

```
Variants/<Name>/
├── manifest.json   # identity + packing parameters (see below)
└── config.json     # the embedded VariantConfig JSON (schema/name/fit/features)
```

CI packs every manifest after the app build (`pack_variants.py` driving `make_variant.py`)
and publishes the resulting `.uapp` exactly like a compiled app: an `app-<Name>` artifact
and a `<Name>/` folder inside the `una-apps-*.zip` release. Variants are NOT part of the
compiled-app matrix (no `*-CMake` directory) by design.

## manifest.json fields

| Field | Meaning |
|---|---|
| `name` | Launcher name (ASCII, max 15 chars) and output file stem |
| `appid` | The variant's OWN unique 16-hex uappID — never the target's (see allocation below) |
| `target` | Directory name of the base app under `Examples/Apps/` (e.g. `Hiking`). The target's uappID is read from its freshly built `.uapp` at pack time, so it can never drift from the binary |
| `type` | App type — must match the target's (`Activity`/`Utility`/`Clockface`; glance targets are rejected by the kernel) |
| *(version)* | Not a manifest field: variants carry the **same version as their target app** — derived at pack time from the freshly built target `.uapp`, so everything in one `una-apps-*` release shares one version. A config-only change still ships under the next release's version |
| `min_target_version` | Minimum target app version, `0.0.0` = any |
| `origin` | `shipped` for everything in this tree (`user` is reserved for on-watch CreateVariant) |
| `config` | Path (relative to the manifest) of the embedded config JSON; must carry `"schema": 1` |
| `icons` | `"target"` = copy both icons out of the built target `.uapp` (the v1 convention), or `{"normal": "icon_60x60.png", "small": "icon_30x30.png"}` for custom PNGs |

## AppID allocation rule

Variant uappIDs live in the same 16-hex namespace as app `APP_ID`s
(`Examples/Apps/<App>/Software/App*/*-CMake/CMakeLists.txt`):

1. Generate a random 16-hex value (e.g. `python3 -c "import secrets; print(secrets.token_hex(8).upper())"`).
2. It must collide with **nothing**: any app's `APP_ID` or any other variant manifest.
   `pack_variants.py` enforces this on every CI run (it reads the IDs out of all built
   `.uapp` headers and all manifests) and the kernel independently rejects a collision at
   boot scan — but catch it here, not on a customer's watch.
3. Once shipped, an ID is permanent: the phone's update/uninstall bookkeeping keys on it.
   Bump `appver` for content changes; never reuse or rotate the ID.

Current allocations:

| Variant | appid | Target |
|---|---|---|
| Walking | `A1E5D3B7C9F04A82` | Hiking (`A1F3C92B7E4D8A10`) |

## Local build

```bash
# after building the target app (e.g. Hiking) so its .uapp exists:
python3 Utilities/Scripts/app_merging/pack_variants.py \
    --apps-root Examples/Apps --out Output/variants
```
