#!/usr/bin/env python3
"""
Watch SDK Build Tool
Similar to ESP-IDF's idf.py, provides unified interface for watch app development
"""

import os
import sys
import subprocess
import argparse
import shutil
from pathlib import Path

def check_watch_sdk_path():
    """Check if UNA_SDK is set"""
    sdk_path = os.environ.get('UNA_SDK')
    if not sdk_path:
        print("ERROR: UNA_SDK environment variable not set")
        print("Please set it to the watch SDK path:")
        print("export UNA_SDK=/path/to/watch-sdk")
        sys.exit(1)
    return Path(sdk_path)

def run_command(cmd, cwd=None, check=True):
    """Run a command and return the result"""
    print(f"Running: {' '.join(cmd)}")
    if cwd:
        print(f"In directory: {cwd}")
    return subprocess.run(cmd, cwd=cwd, check=check)

def cmake_configure(build_dir, source_dir, extra_args=None):
    """Configure CMake project"""
    cmd = ['cmake', '-S', str(source_dir), '-B', str(build_dir)]
    if extra_args:
        cmd.extend(extra_args)
    run_command(cmd)

def cmake_build(build_dir, target=None):
    """Build CMake project"""
    cmd = ['cmake', '--build', str(build_dir)]
    if target:
        cmd.extend(['--target', target])
    run_command(cmd)

def create_app_template(app_name, template_path=None):
    """Create a new app from template"""
    if template_path is None:
        # Use Running app as template
        template_path = Path("Apps/Running")

    app_dir = Path(f"apps/{app_name}")
    if app_dir.exists():
        print(f"ERROR: App directory {app_dir} already exists")
        return False

    print(f"Creating app {app_name} from template...")
    shutil.copytree(template_path, app_dir)

    # Update CMakeLists.txt with app name
    cmake_file = app_dir / "Software" / "Apps" / f"{app_name}-CMake" / "CMakeLists.txt"
    if cmake_file.exists():
        content = cmake_file.read_text()
        content = content.replace('project(Running)', f'project({app_name})')
        content = content.replace('set(APP_NAME "Running")', f'set(APP_NAME "{app_name}")')
        # Generate a simple app ID based on name
        app_id = f"{hash(app_name) % 0xFFFFFFFFFFFFFFFF:016X}"[:16].upper()
        content = content.replace('set(APP_ID "A12E9F4C8B7D3A65")', f'set(APP_ID "{app_id}")')
        cmake_file.write_text(content)

    print(f"App {app_name} created successfully in {app_dir}")
    return True

def main():
    parser = argparse.ArgumentParser(description='Watch SDK Build Tool')
    subparsers = parser.add_subparsers(dest='command', help='Available commands')

    # create-app command
    create_parser = subparsers.add_parser('create-app', help='Create a new app from template')
    create_parser.add_argument('name', help='App name')
    create_parser.add_argument('--template', help='Template app path')

    # build command
    build_parser = subparsers.add_parser('build', help='Build the app')
    build_parser.add_argument('--clean', action='store_true', help='Clean build')
    build_parser.add_argument('--target', help='Build target')

    # clean command
    subparsers.add_parser('clean', help='Clean build directory')

    # Other commands
    subparsers.add_parser('reconfigure', help='Reconfigure CMake')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return

    # Check SDK path for most commands
    if args.command not in ['create-app']:
        sdk_path = check_watch_sdk_path()

    # Get current directory as app directory
    app_dir = Path.cwd()

    if args.command == 'create-app':
        create_app_template(args.name, args.template)
        return

    # Check if we're in an app directory
    cmake_file = app_dir / "CMakeLists.txt"
    if not cmake_file.exists():
        print("ERROR: No CMakeLists.txt found in current directory")
        print("Please run this command from an app directory")
        sys.exit(1)

    build_dir = app_dir / "build"

    if args.command == 'clean':
        if build_dir.exists():
            shutil.rmtree(build_dir)
            print("Build directory cleaned")
        return

    if args.command == 'reconfigure':
        if build_dir.exists():
            shutil.rmtree(build_dir)
        cmake_configure(build_dir, app_dir)
        return

    if args.command == 'build':
        # Configure if build dir doesn't exist
        if not build_dir.exists() or args.clean:
            if build_dir.exists():
                shutil.rmtree(build_dir)
            cmake_configure(build_dir, app_dir)

        # Build
        cmake_build(build_dir, args.target)
        return

if __name__ == '__main__':
    main()