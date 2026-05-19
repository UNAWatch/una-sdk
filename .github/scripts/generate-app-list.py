#!/usr/bin/env python3
import os
import json
import sys

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

if len(sys.argv) > 1 and sys.argv[1] == '--app':
    if len(sys.argv) < 3:
        print("Error: --app requires an app name", file=sys.stderr)
        sys.exit(1)
    app_name = sys.argv[2]
    if app_name in apps_excluded:
        # App is excluded, output nothing
        sys.exit(0)
    projects = []
    for root, dirs, files in os.walk(os.path.join(base_path, app_name)):
        # Exclude certain directories
        dirs[:] = [d for d in dirs if not should_exclude_dir(d)]
        for dir_name in dirs:
            if dir_name.endswith('-CubeIDE'):
                projects.append(os.path.join(root, dir_name))
    print('\n'.join(projects))
else:
    cmake_apps = set()
    cubeide_apps = set()

    for root, dirs, files in os.walk(base_path):
        # Exclude certain directories
        dirs[:] = [d for d in dirs if not should_exclude_dir(d)]
        for dir_name in dirs:
            if dir_name.endswith('-CMake'):
                # Get the app name: the directory name under Apps/
                rel_path = os.path.relpath(root, base_path)
                if '/' in rel_path:
                    app_name = rel_path.split('/')[0]
                else:
                    # If root is directly Apps/, but shouldn't happen
                    continue
                if app_name not in apps_excluded:
                    cmake_apps.add(app_name)
            elif dir_name.endswith('-CubeIDE'):
                rel_path = os.path.relpath(root, base_path)
                if '/' in rel_path:
                    app_name = rel_path.split('/')[0]
                else:
                    continue
                if app_name not in apps_excluded:
                    cubeide_apps.add(app_name)

    # Output as JSON
    output = {
        'cmake_apps': sorted(list(cmake_apps)),
        'cubeide_apps': sorted(list(cubeide_apps))
    }
    print(json.dumps(output))
