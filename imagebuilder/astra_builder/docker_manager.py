"""
Docker operations module.
Handles Docker image building and container lifecycle management.
"""

import subprocess
import logging
from pathlib import Path
from typing import Optional

from .config import ConfigManager


class DockerOperationError(Exception):
    """Raised when a Docker operation fails."""
    pass


class DockerManager:
    """Manages Docker operations for the Astra project."""
    
    def __init__(self, config: ConfigManager, logger: Optional[logging.Logger] = None):
        """
        Initialize Docker manager.
        
        Args:
            config: Configuration manager instance
            logger: Logger instance (optional)
        """
        self.config = config
        self.logger = logger or logging.getLogger(__name__)
        
        self.script_dir = Path(__file__).parent.parent.absolute()
        self.project_root = self.script_dir.parent
        
        self.logger.debug(f"Script directory: {self.script_dir}")
        self.logger.debug(f"Project root: {self.project_root}")
    
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
            DockerOperationError: If command fails and check=True
        """
        self.logger.debug(f"Executing: {command}")
        if cwd:
            self.logger.debug(f"Working directory: {cwd}")
        
        try:
            result = subprocess.run(
                command,
                shell=True,
                check=check,
                cwd=cwd,
                capture_output=False,
                text=True
            )
            self.logger.debug(f"Command completed with exit code: {result.returncode}")
            return result
            
        except subprocess.CalledProcessError as e:
            self.logger.error(f"Command failed with exit code {e.returncode}")
            raise DockerOperationError(f"Docker command failed: {command}") from e
    
    def build_image(self) -> None:
        """
        Build the Docker image.
        
        Raises:
            FileNotFoundError: If Dockerfile not found
            DockerOperationError: If build fails
        """
        image_name = self.config['image_name']
        dockerfile_path = self.script_dir / self.config['dockerfile_path']
        
        self.logger.info(f"Building Docker image: {image_name}")
        
        if not (dockerfile_path / 'Dockerfile').exists():
            raise FileNotFoundError(f"Dockerfile not found in {dockerfile_path}")
        
        command = f"docker build -t {image_name} {self.config['dockerfile_path']}"
        self._run_command(command, cwd=self.script_dir)
        
        self.logger.info(f"Successfully built image: {image_name}")
    
    def start_container(self) -> None:
        """
        Start the Docker container.
        
        Raises:
            DockerOperationError: If container start fails
        """
        container_name = self.config['container_name']
        
        self.logger.info(f"Starting container: {container_name}")
        
        # Remove existing container
        self.logger.debug("Removing existing container if present")
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
        self.logger.info(f"Successfully started container: {container_name}")
    
    def stop_container(self) -> None:
        """
        Stop and remove the Docker container.
        
        Raises:
            DockerOperationError: If stop fails
        """
        container_name = self.config['container_name']
        
        self.logger.info(f"Stopping container: {container_name}")
        self._run_command(f"docker rm -f {container_name}", check=False)
        self.logger.info(f"Container stopped: {container_name}")
    
    def setup_environment(self) -> None:
        """
        Perform complete environment setup (build + start).
        
        Raises:
            FileNotFoundError: If Dockerfile not found
            DockerOperationError: If any operation fails
        """
        self.logger.info("Setting up complete Docker environment")
        self.build_image()
        self.start_container()
        self.logger.info("Environment setup complete")
