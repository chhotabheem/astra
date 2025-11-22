#!/usr/bin/env python3
import subprocess
import argparse
import os
import sys

# Configuration
IMAGE_NAME = "astrabuilder"
CONTAINER_NAME = "prayag"
PROJECT_ROOT = os.getcwd()
CONTAINER_WORKDIR = "/app/astra"

def run_command(command, cwd=None, check=True):
    """Run a shell command and print output."""
    print(f"Running: {command}")
    try:
        subprocess.run(command, shell=True, check=check, cwd=cwd)
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {command}")
        sys.exit(1)

def build_image():
    """Build the Docker image."""
    print(f"Building Docker image: {IMAGE_NAME}...")
    run_command(f"docker build -t {IMAGE_NAME} .", cwd=os.path.join(PROJECT_ROOT, "imagebuilder"))

def start_container():
    """Start the Docker container."""
    print(f"Starting container: {CONTAINER_NAME}...")
    # Remove existing container if it exists
    run_command(f"docker rm -f {CONTAINER_NAME}", check=False)
    
    # Run new container
    # Use --network host to ensure connectivity for downloads if needed, 
    # though bridge should work if DNS is fine. Sticking to default bridge for isolation unless issues arise.
    # But previously I used host network to fix download issues. Let's use host network to be safe.
    cmd = (
        f"docker run -d --name {CONTAINER_NAME} --network host "
        f"-v {PROJECT_ROOT}:{CONTAINER_WORKDIR} "
        f"{IMAGE_NAME} tail -f /dev/null"
    )
    run_command(cmd)

def build_code():
    """Compile and test the code inside the container."""
    print("Compiling and testing code...")
    
    build_cmd = (
        f"cd {CONTAINER_WORKDIR}/mongoclient && "
        "mkdir -p build && "
        "cd build && "
        "cmake .. && "
        "cmake --build . && "
        "ctest --output-on-failure && "
        "./mongo_app"
    )
    
    run_command(f"docker exec -it {CONTAINER_NAME} /bin/bash -c '{build_cmd}'")

def clean():
    """Clean up containers and images."""
    print("Cleaning up...")
    run_command(f"docker rm -f {CONTAINER_NAME}", check=False)

def main():
    parser = argparse.ArgumentParser(description="Astra Build Automation")
    parser.add_argument("--image", action="store_true", help="Build Docker image")
    parser.add_argument("--start", action="store_true", help="Start Docker container")
    parser.add_argument("--build", action="store_true", help="Compile and test code")
    parser.add_argument("--clean", action="store_true", help="Clean up container")
    parser.add_argument("--all", action="store_true", help="Run full pipeline (image -> start -> build)")
    
    args = parser.parse_args()
    
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    if args.clean:
        clean()
        
    if args.image or args.all:
        build_image()
        
    if args.start or args.all:
        start_container()
        
    if args.build or args.all:
        build_code()

if __name__ == "__main__":
    main()
