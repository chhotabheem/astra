#!/usr/bin/env python3
import subprocess
import argparse
import os
import sys

# Configuration
IMAGE_NAME = "astrabuilder"
CONTAINER_NAME = "prayag"

# Determine paths
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
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
    # Run build from imagebuilder directory
    run_command(f"docker build -t {IMAGE_NAME} .", cwd=SCRIPT_DIR)

def start_container():
    """Start the Docker container."""
    print(f"Starting container: {CONTAINER_NAME}...")
    # Remove existing container if it exists
    run_command(f"docker rm -f {CONTAINER_NAME}", check=False)
    
    # Run new container
    cmd = (
        f"docker run -d --name {CONTAINER_NAME} --network host "
        f"-v {PROJECT_ROOT}:{CONTAINER_WORKDIR} "
        f"{IMAGE_NAME} tail -f /dev/null"
    )
    run_command(cmd)

def stop_container():
    """Stop the Docker container."""
    print(f"Stopping container: {CONTAINER_NAME}...")
    run_command(f"docker rm -f {CONTAINER_NAME}", check=False)

def main():
    parser = argparse.ArgumentParser(description="Astra Environment Management")
    parser.add_argument("--build", action="store_true", help="Build Docker image")
    parser.add_argument("--start", action="store_true", help="Start Docker container")
    parser.add_argument("--stop", action="store_true", help="Stop Docker container")
    parser.add_argument("--up", action="store_true", help="Build image and start container")
    
    args = parser.parse_args()
    
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    if args.stop:
        stop_container()
        
    if args.build or args.up:
        build_image()
        
    if args.start or args.up:
        start_container()

if __name__ == "__main__":
    main()
