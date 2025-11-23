"""
Docker operations and configuration management.
"""

import json
import subprocess
from pathlib import Path
from typing import Dict, Optional


class DockerError(Exception):
    """Raised when a Docker operation fails."""
    pass


class DockerManager:
    """Manages Docker operations for the Astra project."""
    
    def __init__(self, config_path: Optional[Path] = None):
        """
        Initialize Docker manager.
        
        Args:
            config_path: Path to configuration file
        """
        if config_path is None:
            config_path = Path(__file__).parent.parent / "config.json"
        
        self.config = self._load_config(config_path)
        self.script_dir = Path(__file__).parent.parent.absolute()
        self.project_root = self.script_dir.parent
    
    def _load_config(self, config_path: Path) -> Dict:
        """Load configuration from JSON file."""
        try:
            with open(config_path, 'r') as f:
                config = json.load(f)
        except FileNotFoundError:
            raise DockerError(f"Configuration file not found: {config_path}")
        except json.JSONDecodeError as e:
            raise DockerError(f"Invalid JSON in configuration file: {e}")
        
        # Apply defaults
        config.setdefault('network_mode', 'host')
        
        return config
    
    def _run_command(self, command: str, cwd: Optional[Path] = None, 
                     check: bool = True) -> subprocess.CompletedProcess:
        """
        Execute a shell command.
        
        Args:
            command: Command to execute
            cwd: Working directory
            check: Whether to raise exception on failure
            
        Returns:
            CompletedProcess instance
            
        Raises:
            DockerError: If command fails and check=True
        """
        print(f"→ {command}")
        
        try:
            result = subprocess.run(
                command,
                shell=True,
                check=check,
                cwd=cwd,
                capture_output=False,
                text=True
            )
            return result
            
        except subprocess.CalledProcessError as e:
            raise DockerError(f"Command failed: {command}") from e
    
    def build_image(self) -> None:
        """
        Build the Docker image.
        
        Raises:
            FileNotFoundError: If Dockerfile not found
            DockerError: If build fails
        """
        image_name = self.config['image_name']
        dockerfile_path = self.script_dir / '.'
        build_network = self.config.get('network_mode', 'host')
        
        print(f"Building Docker image: {image_name}")
        print(f"Network mode: {build_network}")
        
        if not (dockerfile_path / 'Dockerfile').exists():
            raise FileNotFoundError(f"Dockerfile not found in {dockerfile_path}")
        
        # Build command with network flag
        network_flag = f"--network={build_network}" if build_network else ""
        command = f"docker build {network_flag} -t {image_name} -f Dockerfile {self.project_root}"
        
        self._run_command(command, cwd=dockerfile_path)
        
        print(f"✅ Successfully built image: {image_name}")
    
    def start_container(self) -> None:
        """
        Start the Docker container.
        
        Raises:
            DockerError: If container start fails
        """
        container_name = self.config['container_name']
        
        print(f"Starting container: {container_name}")
        
        # Remove existing container
        self._run_command(f"docker rm -f {container_name}", check=False)
        
        # Build run command
        command = (
            f"docker run -d "
            f"--name {container_name} "
            f"--network {self.config['network_mode']} "
            f"-v {self.project_root}:{self.config['container_workdir']} "
            f"{self.config['image_name']} "
            f"tail -f /dev/null"
        )
        
        self._run_command(command)
        print(f"✅ Successfully started container: {container_name}")
    
    def stop_container(self) -> None:
        """
        Stop and remove the Docker container.
        
        Raises:
            DockerError: If stop fails
        """
        container_name = self.config['container_name']
        
        print(f"Stopping container: {container_name}")
        self._run_command(f"docker rm -f {container_name}", check=False)
        print(f"✅ Container stopped: {container_name}")
    
    def get(self, key: str, default=None):
        """Get configuration value by key."""
        return self.config.get(key, default)
    
    def __getitem__(self, key: str):
        """Get configuration value using dictionary syntax."""
        return self.config[key]
