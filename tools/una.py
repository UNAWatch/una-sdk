#!/usr/bin/env python3
"""
Watch SDK Build Tool
una.py provides unified interface for watch app development
"""

import os
import sys
import subprocess
import argparse
import shutil
from pathlib import Path

def get_sdk_path():
    """Get SDK path - either from environment or current directory detection"""
    # First check if UNA_SDK is set (for external apps)
    sdk_path = os.environ.get('UNA_SDK')
    if sdk_path:
        return Path(sdk_path)

    # Check if we're within SDK repository - search upwards
    current_dir = Path.cwd()
    for search_dir in [current_dir] + list(current_dir.parents):
        # Look for SDK structure
        if (search_dir / "cmake" / "watch_project.cmake").exists():
            return search_dir
        elif (search_dir / "SDK" / "cmake" / "watch_project.cmake").exists():
            return search_dir / "SDK"

    print("ERROR: Cannot determine SDK path.")
    print("Either set UNA_SDK environment variable or run from within SDK repository")
    print("export UNA_SDK=/path/to/una-watch-sdk")
    sys.exit(1)

def run_command(cmd, cwd=None, check=True):
    """Run a command and return the result"""
    print(f"Running: {' '.join(cmd)}")
    if cwd:
        print(f"In directory: {cwd}")
    return subprocess.run(cmd, cwd=cwd, check=check)

def cmake_configure(build_dir, source_dir, extra_args=None):
    """Configure CMake project"""
    sdk_path = get_sdk_path()
    env = os.environ.copy()
    env['UNA_SDK'] = str(sdk_path)
    cmd = ['cmake', '-S', str(source_dir), '-B', str(build_dir)]
    if extra_args:
        cmd.extend(extra_args)
    subprocess.run(cmd, cwd=None, check=True, env=env)

def cmake_build(build_dir, target=None):
    """Build CMake project"""
    cmd = ['cmake', '--build', str(build_dir)]
    if target:
        cmd.extend(['--target', target])
    run_command(cmd)

def export_sdk():
    """Export UNA_SDK environment variable"""
    sdk_path = get_sdk_path()
    print(f"export UNA_SDK={sdk_path}")
    print("Add this to your shell profile or run it before building apps")

def init_env():
    """Interactive setup of .env file"""
    print("Interactive .env setup")
    print("======================")

    # Ask for app type
    app_type = input("App type (service-only or service+gui): ").strip()
    if app_type not in ['service-only', 'service+gui']:
        print("ERROR: Invalid app type. Must be 'service-only' or 'service+gui'")
        return False

    # Common paths
    libs_path = input("Relative path to Libs directory (e.g., ../../Libs): ").strip()
    output_path = input("Relative path to Output directory (e.g., ../../../Output): ").strip()
    resources_path = input("Relative path to Resources directory (e.g., ../../../Resources): ").strip()

    touchgfx_gui_path = ""
    if app_type == 'service+gui':
        touchgfx_gui_path = input("Relative path to TouchGFX-GUI directory (e.g., ../TouchGFX-GUI): ").strip()

    # Create .env content
    env_content = f"""LIBS_PATH={libs_path}
OUTPUT_PATH={output_path}
RESOURCES_PATH={resources_path}
APP_PATH=.
APP_TYPE={app_type}
"""
    if app_type == 'service+gui':
        env_content += f"TOUCHGFX_GUI_PATH={touchgfx_gui_path}\n"

    # Write .env file
    env_file = Path(".env")
    if env_file.exists():
        overwrite = input(".env file already exists. Overwrite? (y/N): ").strip().lower()
        if overwrite != 'y':
            print("Aborted")
            return False

    env_file.write_text(env_content)
    print(f".env file created successfully")
    return True

def create_app(app_name, app_type):
    """Create a new app from template"""
    sdk_path = get_sdk_path()

    if app_type not in ['service-only', 'service-gui']:
        print("ERROR: Invalid app type. Must be 'service-only' or 'service-gui'")
        return False

    app_dir = Path(app_name)
    if app_dir.exists():
        print(f"ERROR: App directory {app_dir} already exists")
        return False

    # Create directory structure
    cmake_dir = app_dir / "Software" / "Apps" / f"{app_name}-CMake"
    cmake_dir.mkdir(parents=True, exist_ok=True)

    # Copy CMakeLists.txt from template
    template_file = sdk_path / "templates" / app_type / "CMakeLists.txt"
    if not template_file.exists():
        print(f"ERROR: Template file not found: {template_file}")
        return False

    cmake_file = cmake_dir / "CMakeLists.txt"
    shutil.copy(template_file, cmake_file)

    # Copy common files
    # Copy syscalls.cpp from SDK examples
    syscalls_src = sdk_path / "examples" / "Apps" / "Running" / "Software" / "Apps" / "Running-CMake" / "syscalls.cpp"
    if syscalls_src.exists():
        shutil.copy(syscalls_src, cmake_dir / "syscalls.cpp")

    # Copy linker scripts
    if app_type == 'service-only':
        linker_src = sdk_path / "examples" / "Apps" / "Running" / "Software" / "Apps" / "Running-CMake" / "RunningService.ld"
        if linker_src.exists():
            shutil.copy(linker_src, cmake_dir / f"{app_name}Service.ld")
    else:
        # For service-gui, copy both
        service_linker_src = sdk_path / "examples" / "Apps" / "Running" / "Software" / "Apps" / "Running-CMake" / "RunningService.ld"
        gui_linker_src = sdk_path / "examples" / "Apps" / "Running" / "Software" / "Apps" / "Running-CMake" / "RunningGUI.ld"
        if service_linker_src.exists():
            shutil.copy(service_linker_src, cmake_dir / f"{app_name}Service.ld")
        if gui_linker_src.exists():
            shutil.copy(gui_linker_src, cmake_dir / f"{app_name}GUI.ld")

    # Copy PaintImpl.cpp for GUI apps
    if app_type == 'service-gui':
        paint_src = sdk_path / "examples" / "Apps" / "Running" / "Software" / "Apps" / "Running-CMake" / "PaintImpl.cpp"
        if paint_src.exists():
            shutil.copy(paint_src, cmake_dir / "PaintImpl.cpp")

    # Customize CMakeLists.txt
    content = cmake_file.read_text()
    content = content.replace('project(MyApp)', f'project({app_name})')
    content = content.replace('set(APP_NAME "MyApp")', f'set(APP_NAME "{app_name}")')
    # Generate a simple app ID based on name
    app_id = f"{hash(app_name) % 0xFFFFFFFFFFFFFFFF:016X}"[:16].upper()
    content = content.replace('set(APP_ID "your_app_id_here")', f'set(APP_ID "{app_id}")')
    # Update linker script names
    content = content.replace('RunningService.ld', f'{app_name}Service.ld')
    if app_type == 'service-gui':
        content = content.replace('RunningGUI.ld', f'{app_name}GUI.ld')
    cmake_file.write_text(content)

    # Create .env template
    env_file = cmake_dir / ".env"
    if app_type == 'service-only':
        env_content = """LIBS_PATH=../../Libs
OUTPUT_PATH=../../../Output
RESOURCES_PATH=../../../Resources
APP_PATH=.
APP_TYPE=service-only
"""
    else:
        env_content = """LIBS_PATH=../../Libs
TOUCHGFX_GUI_PATH=../TouchGFX-GUI
OUTPUT_PATH=../../../Output
RESOURCES_PATH=../../../Resources
APP_PATH=.
APP_TYPE=service+gui
"""
    env_file.write_text(env_content)

    print(f"App {app_name} created successfully in {app_dir}")
    print("Next steps:")
    print("1. Edit the .env file in Software/Apps/{app_name}-CMake/ to set correct relative paths")
    print("2. Implement your app logic in Software/Libs/")
    if app_type == 'service-gui':
        print("3. Set up TouchGFX project in Software/Apps/TouchGFX-GUI/")
    print("4. Run 'una init' to interactively configure paths if needed")
    return True

def create_app_template(app_name, template_path=None):
    """Create a new app from template"""
    if template_path is None:
        # Use Running app as template
        template_path = Path("examples/Apps/Running")

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

def build_example(example_name):
    """Build an example app within the SDK"""
    sdk_path = get_sdk_path()
    example_path = sdk_path / "examples" / "Apps" / example_name

    if not example_path.exists():
        print(f"ERROR: Example {example_name} not found in {example_path}")
        return False

    cmake_dir = example_path / "Software" / "Apps" / f"{example_name}-CMake"
    if not cmake_dir.exists():
        print(f"ERROR: CMake directory not found: {cmake_dir}")
        return False

    build_dir = cmake_dir / "build"

    print(f"Building example {example_name}...")

    # Configure
    if not build_dir.exists():
        cmake_configure(build_dir, cmake_dir)

    # Build
    cmake_build(build_dir)

    print(f"Example {example_name} built successfully")
    return True

def main():
    parser = argparse.ArgumentParser(description='Una-Watch SDK Build Tool')
    subparsers = parser.add_subparsers(dest='command', help='Available commands')

    # export command
    subparsers.add_parser('export', help='Print export command for UNA_SDK environment variable')

    # init command
    subparsers.add_parser('init', help='Interactive setup of .env file')

    # create command
    create_parser = subparsers.add_parser('create', help='Create a new app from template')
    create_parser.add_argument('name', help='App name')
    create_parser.add_argument('type', choices=['service-only', 'service-gui'], help='App type')

    # build-example command
    build_example_parser = subparsers.add_parser('build-example', help='Build an example app within SDK')
    build_example_parser.add_argument('name', help='Example app name (e.g., Running)')

    # create-app command (legacy)
    create_app_parser = subparsers.add_parser('create-app', help='Create a new app from existing app template')
    create_app_parser.add_argument('name', help='App name')
    create_app_parser.add_argument('--template', help='Template app path')

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

    if args.command == 'export':
        export_sdk()
        return

    if args.command == 'init':
        init_env()
        return

    if args.command == 'create':
        create_app(args.name, args.type)
        return

    if args.command == 'build-example':
        build_example(args.name)
        return

    if args.command == 'create-app':
        create_app_template(args.name, args.template)
        return

    # For other commands, get current directory as app directory
    app_dir = Path.cwd()

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