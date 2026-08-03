#!/usr/bin/env python3
import os
import json

script_dir = os.path.dirname(os.path.abspath(__file__))
repo_root = os.path.normpath(os.path.join(script_dir, "../.."))
base_path = os.path.join(repo_root, "Examples", "Apps")


def should_exclude_dir(dir_name):
    # Exclude hidden directories, generated content, output, and build directories
    exclude_dirs = {'.git', 'generated', 'Output', 'build', 'simulator', 'node_modules', '.vscode', '.github'}
    return dir_name.startswith('.') or dir_name in exclude_dirs


# Read APPS_EXCLUDED environment variable
apps_excluded = set()
excluded_env = os.environ.get('APPS_EXCLUDED', '').strip()
if excluded_env:
    apps_excluded = set(line.strip() for line in excluded_env.split('\n') if line.strip())

apps = set()

for root, dirs, files in os.walk(base_path):
    dirs[:] = [d for d in dirs if not should_exclude_dir(d)]
    for dir_name in dirs:
        if dir_name.endswith('-CMake'):
            rel_path = os.path.relpath(root, base_path)
            parts = rel_path.split(os.sep)
            if len(parts) >= 1 and parts[0] and parts[0] != '.':
                app_name = parts[0]
                if app_name not in apps_excluded and app_name != 'Variants':
                    apps.add(app_name)

# Shipped variants: code-less alias .uapps packed after the app build (no
# -CMake dir, so they can never appear in the compiled-app matrix above).
# See Examples/Apps/Variants/README.md.
variants = set()
variants_path = os.path.join(base_path, 'Variants')
if os.path.isdir(variants_path):
    for dir_name in sorted(os.listdir(variants_path)):
        if os.path.isfile(os.path.join(variants_path, dir_name, 'manifest.json')):
            if dir_name not in apps_excluded:
                variants.add(dir_name)

output = {'apps': sorted(list(apps)), 'variants': sorted(list(variants))}
print(json.dumps(output))
