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
    """Get SDK path - either from environment or script location detection"""
    # First check if UNA_SDK is set (for external apps)
    sdk_path = os.environ.get('UNA_SDK')
    if sdk_path:
        return Path(sdk_path)

    # Use script location for SDK detection
    script_path = Path(__file__).resolve().parent
    # Check if script is in tools/ directory
    if (script_path.parent / "cmake" / "watch_project.cmake").exists():
        return script_path.parent
    # elif (script_path / "cmake" / "watch_project.cmake").exists():
    #     return script_path

    print("ERROR: Cannot determine SDK path.")
    print("Either set UNA_SDK environment variable or run una.py from within SDK repository")
    print("export UNA_SDK=/path/to/una-watch-sdk")
    sys.exit(1)

def run_command(cmd, cwd=None, check=True):
    """Run a command and return the result"""
    print(f"Running: {' '.join(cmd)}")
    if cwd:
        print(f"In directory: {cwd}")
    return subprocess.run(cmd, cwd=cwd, check=check)

def normalize_app_type(app_type):
    if app_type == 'service-gui':
        return 'service+gui'
    return app_type

def generate_app_id(app_name):
    return f"{hash(app_name) % 0xFFFFFFFFFFFFFFFF:016X}"[:16].upper()

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
    print(f"export PATH={os.environ.get('PATH')}:{sdk_path}/tools")
    print("\n# Add this to your shell profile or run $(una.py export)")

def init_env():
    """Interactive setup of .env file"""
    print("Interactive .env setup")
    print("======================")

    # Ask for app type
    app_type = normalize_app_type(input("App type (service-only or service+gui): ").strip())
    if app_type not in ['service-only', 'service+gui']:
        print("ERROR: Invalid app type. Must be 'service-only' or 'service+gui'")
        return False

    def prompt_default(prompt, default):
        value = input(f"{prompt} [{default}]: ").strip()
        return value or default

    def prompt_optional(prompt):
        value = input(f"{prompt} (leave empty to skip): ").strip()
        return value or ""

    # Common paths
    app_path = prompt_default("Relative path to app root (APP_PATH)", ".")
    libs_path = prompt_default("Relative path to Libs directory", "Libs")
    output_path = prompt_default("Relative path to Output directory", "Output")
    resources_path = prompt_default("Relative path to Resources directory", "Resources")

    touchgfx_gui_path = ""
    if app_type == 'service+gui':
        touchgfx_gui_path = prompt_default("Relative path to TouchGFX-GUI directory", "TouchGFX-GUI")

    configure_linker = input("Configure linker STACK_SIZE/RAM_LENGTH? (y/N): ").strip().lower() == 'y'
    service_stack_size = ""
    service_ram_length = ""
    gui_stack_size = ""
    gui_ram_length = ""
    if configure_linker:
        service_stack_size = prompt_optional("Service STACK_SIZE (e.g., 10*1024)")
        service_ram_length = prompt_optional("Service RAM_LENGTH (e.g., 500K)")
        if app_type == 'service+gui':
            gui_stack_size = prompt_optional("GUI STACK_SIZE (e.g., 10*1024)")
            gui_ram_length = prompt_optional("GUI RAM_LENGTH (e.g., 600K)")

    # Create .env content
    env_content = f"""LIBS_PATH={libs_path}
TOUCHGFX_GUI_PATH={touchgfx_gui_path}
OUTPUT_PATH={output_path}
RESOURCES_PATH={resources_path}
APP_PATH={app_path}
APP_TYPE={app_type}
"""
    if app_type != 'service+gui':
        env_content = env_content.replace("TOUCHGFX_GUI_PATH=\n", "")
    if service_stack_size:
        env_content += f"SERVICE_STACK_SIZE={service_stack_size}\n"
    if service_ram_length:
        env_content += f"SERVICE_RAM_LENGTH={service_ram_length}\n"
    if gui_stack_size:
        env_content += f"GUI_STACK_SIZE={gui_stack_size}\n"
    if gui_ram_length:
        env_content += f"GUI_RAM_LENGTH={gui_ram_length}\n"

    # Write .env file in current directory
    env_file = Path.cwd() / ".env"
    if env_file.exists():
        overwrite = input(".env file already exists. Overwrite? (y/N): ").strip().lower()
        if overwrite != 'y':
            print("Aborted")
            return False

    env_file.write_text(env_content)
    print(f".env file created successfully")
    return True

def copy_directory_contents(source_dir, target_dir, force=False, skip_names=None):
    """Copy all contents of source_dir into target_dir"""
    skip_names = set(skip_names or [])
    for item in source_dir.iterdir():
        if item.name in skip_names:
            continue
        dest = target_dir / item.name
        if dest.exists() and not force:
            raise FileExistsError(f"Target already contains {dest}")
        if item.is_dir():
            shutil.copytree(item, dest, dirs_exist_ok=force)
        else:
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(item, dest)

def find_template_conflicts(template_dir, target_dir, skip_names=None):
    skip_names = set(skip_names or [])
    conflicts = []
    for item in template_dir.iterdir():
        if item.name in skip_names:
            continue
        dest = target_dir / item.name
        if dest.exists():
            conflicts.append(dest)
    return conflicts

def resolve_candidate_paths(paths, sdk_path):
    """Return candidate Path objects for provided paths, including SDK-relative fallbacks."""
    candidates = []
    for entry in paths or []:
        raw = Path(entry)
        candidates.append(raw)
        if not raw.is_absolute():
            candidates.append(sdk_path / raw)
    return candidates

def select_existing_dir(candidates):
    for candidate in candidates:
        if candidate.exists() and candidate.is_dir():
            return candidate
    return None

def find_example_touchgfx_dir(sdk_path):
    examples_root = sdk_path / "examples" / "Apps"
    if not examples_root.exists():
        return None
    touchgfx_dirs = sorted(examples_root.glob("*/Software/Apps/TouchGFX-GUI"))
    return touchgfx_dirs[0] if touchgfx_dirs else None

def create_app(app_name, app_type, target_path=None, legacy_libs_paths=None, legacy_touchgfx_paths=None, force=False, subdir=False):
    """Create a new app from template"""
    sdk_path = get_sdk_path()

    app_type = normalize_app_type(app_type)
    if app_type not in ['service-only', 'service+gui']:
        print("ERROR: Invalid app type. Must be 'service-only' or 'service+gui'")
        return False

    base_dir = Path(target_path) if target_path else Path.cwd()
    app_dir = base_dir / app_name if subdir else base_dir
    if app_dir.exists():
        pass
    app_dir.mkdir(parents=True, exist_ok=True)

    template_dir = sdk_path / "templates" / app_type
    if not template_dir.exists():
        print(f"ERROR: Template directory not found: {template_dir}")
        return False

    template_skip = {".env"}
    if not force:
        conflicts = find_template_conflicts(template_dir, app_dir, skip_names=template_skip)
        if conflicts:
            print(f"ERROR: Target directory {app_dir} contains existing files. Use an empty directory or pass --force to overwrite existing files.")
            return False
    try:
        copy_directory_contents(template_dir, app_dir, force=force, skip_names=template_skip)
    except FileExistsError as exc:
        print(f"ERROR: {exc}")
        print("Use an empty directory or pass --force to overwrite existing files.")
        return False

    cmake_file = app_dir / "CMakeLists.txt"
    if not cmake_file.exists():
        print("ERROR: Template CMakeLists.txt not found in app root")
        return False

    # Customize CMakeLists.txt
    content = cmake_file.read_text()
    content = content.replace('project(MyApp)', f'project({app_name})')
    content = content.replace('set(APP_NAME "MyApp")', f'set(APP_NAME "{app_name}")')
    app_id = generate_app_id(app_name)
    content = content.replace('set(APP_ID "your_app_id_here")', f'set(APP_ID "{app_id}")')
    cmake_file.write_text(content)


    env_file = app_dir / ".env"
    env_lines = [
        "LIBS_PATH=Libs",
        "OUTPUT_PATH=Output",
        "RESOURCES_PATH=Resources",
        "APP_PATH=.",
        f"APP_TYPE={app_type}",
    ]
    if app_type == 'service+gui':
        env_lines.insert(1, "TOUCHGFX_GUI_PATH=TouchGFX-GUI")
        env_lines.extend([
            "SERVICE_STACK_SIZE=10*1024",
            "SERVICE_RAM_LENGTH=500K",
            "GUI_STACK_SIZE=10*1024",
            "GUI_RAM_LENGTH=600K",
        ])
    else:
        env_lines.extend([
            "SERVICE_STACK_SIZE=10*1024",
            "SERVICE_RAM_LENGTH=500K",
        ])
    if env_file.exists() and not force:
        print(".env already exists, keeping existing file. Use --force to overwrite.")
    else:
        env_file.write_text("\n".join(env_lines) + "\n")

    running_root = sdk_path / "examples" / "Apps" / "Running"
    libs_candidates = resolve_candidate_paths(legacy_libs_paths, sdk_path)
    libs_candidates.append(running_root / "Software" / "Libs")
    libs_source = select_existing_dir(libs_candidates)
    if not libs_source:
        print("ERROR: Running Libs source not found. Provide legacy libs paths if needed.")
        return False
    shutil.copytree(libs_source, app_dir / "Libs", dirs_exist_ok=True)

    if app_type == 'service+gui':
        touchgfx_candidates = resolve_candidate_paths(legacy_touchgfx_paths, sdk_path)
        touchgfx_candidates.append(running_root / "Software" / "Apps" / "TouchGFX-GUI")
        example_touchgfx = find_example_touchgfx_dir(sdk_path)
        if example_touchgfx:
            touchgfx_candidates.append(example_touchgfx)
        touchgfx_source = select_existing_dir(touchgfx_candidates)
        if not touchgfx_source:
            print("ERROR: Running TouchGFX-GUI source not found. Provide legacy TouchGFX paths if needed.")
            return False
        touchgfx_target = app_dir / "TouchGFX-GUI"
        touchgfx_target.mkdir(parents=True, exist_ok=True)
        if touchgfx_source.resolve() != touchgfx_target.resolve():
            copy_directory_contents(touchgfx_source, touchgfx_target, force=True)

    print(f"App {app_name} created successfully in {app_dir}")
    print("Next steps:")
    print("1. Edit the .env file to set correct relative paths if needed")
    print("2. Implement your app logic in Libs/")
    if app_type == 'service+gui':
        print("3. Set up TouchGFX project in TouchGFX-GUI/")
    print("4. Run 'una init' to interactively configure paths if needed")
    return True

def create_app_template(app_name, template_path=None):
    """Create a new app from template"""
    sdk_path = get_sdk_path()
    if template_path is None:
        template_path = sdk_path / "examples" / "Apps" / "Running"

    app_dir = Path(f"apps/{app_name}")
    if app_dir.exists():
        print(f"ERROR: App directory {app_dir} already exists")
        return False

    print(f"Creating app {app_name} from template...")
    shutil.copytree(template_path, app_dir)

    # Update CMakeLists.txt with app name
    cmake_root = app_dir / "Software" / "Apps"
    cmake_dirs = list(cmake_root.glob("*-CMake")) if cmake_root.exists() else []
    cmake_dir = cmake_dirs[0] if cmake_dirs else None
    if cmake_dir and cmake_dir.name != f"{app_name}-CMake":
        cmake_dir = cmake_dir.rename(cmake_root / f"{app_name}-CMake")
    cmake_file = cmake_dir / "CMakeLists.txt" if cmake_dir else None
    if cmake_file and cmake_file.exists():
        content = cmake_file.read_text()
        content = content.replace('project(Running)', f'project({app_name})')
        content = content.replace('set(APP_NAME "Running")', f'set(APP_NAME "{app_name}")')
        app_id = generate_app_id(app_name)
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
    create_parser.add_argument('type', choices=['service-only', 'service+gui', 'service-gui'], help='App type')
    create_parser.add_argument('--path', '-p', help='Target directory for app files (default: current directory)')
    create_parser.add_argument('--subdir', action='store_true', help='Create a subdirectory using the app name inside the target directory')
    create_parser.add_argument('--force', '-f', action='store_true', help='Overwrite existing template files in target directory')
    create_parser.add_argument('--legacy-libs-path', action='append', dest='legacy_libs_paths',
                              help='Legacy Libs source path (can be specified multiple times)')
    create_parser.add_argument('--legacy-touchgfx-path', action='append', dest='legacy_touchgfx_paths',
                              help='Legacy TouchGFX-GUI source path (can be specified multiple times)')

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
        create_app(
            args.name,
            args.type,
            target_path=args.path,
            legacy_libs_paths=args.legacy_libs_paths,
            legacy_touchgfx_paths=args.legacy_touchgfx_paths,
            force=args.force,
            subdir=args.subdir,
        )
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
