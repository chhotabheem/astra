"""
Command-line interface module.
Handles argument parsing and orchestrates operations.
"""

import argparse
import sys
from pathlib import Path
from typing import Optional

from .config import ConfigManager, ConfigurationError
from .logger import setup_logger
from .docker_manager import DockerManager, DockerOperationError


class CLI:
    """Command-line interface for Astra Builder."""
    
    def __init__(self):
        """Initialize CLI."""
        self.parser = self._create_parser()
    
    def _create_parser(self) -> argparse.ArgumentParser:
        """Create argument parser."""
        parser = argparse.ArgumentParser(
            description="Astra Docker Environment Manager",
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog="""
Examples:
  %(prog)s --build              Build the Docker image
  %(prog)s --start              Start the container
  %(prog)s --up                 Build image and start container
  %(prog)s --stop               Stop and remove container
  %(prog)s --config custom.json Use custom configuration file
            """
        )
        
        parser.add_argument('--build', action='store_true',
                           help='Build Docker image')
        parser.add_argument('--start', action='store_true',
                           help='Start Docker container')
        parser.add_argument('--stop', action='store_true',
                           help='Stop Docker container')
        parser.add_argument('--up', action='store_true',
                           help='Build image and start container')
        parser.add_argument('--config', type=str,
                           help='Path to configuration file')
        
        return parser
    
    def run(self, args: Optional[list] = None) -> int:
        """
        Run the CLI application.
        
        Args:
            args: Command-line arguments (None = sys.argv)
            
        Returns:
            Exit code (0 = success, 1 = error)
        """
        parsed_args = self.parser.parse_args(args)
        
        if len(sys.argv) == 1:
            self.parser.print_help()
            return 1
        
        try:
            # Load configuration
            config = ConfigManager(config_path=parsed_args.config)
            
            # Setup logging
            logger = setup_logger(level=config['log_level'])
            logger.info("Astra Builder initialized")
            
            # Create Docker manager
            docker_manager = DockerManager(config, logger)
            
            # Execute requested operations
            if parsed_args.stop:
                docker_manager.stop_container()
            
            if parsed_args.build or parsed_args.up:
                docker_manager.build_image()
            
            if parsed_args.start or parsed_args.up:
                docker_manager.start_container()
            
            return 0
            
        except ConfigurationError as e:
            print(f"Configuration Error: {e}", file=sys.stderr)
            return 1
        except FileNotFoundError as e:
            print(f"File Error: {e}", file=sys.stderr)
            return 1
        except DockerOperationError as e:
            print(f"Docker Operation Error: {e}", file=sys.stderr)
            return 1
        except Exception as e:
            print(f"Unexpected Error: {e}", file=sys.stderr)
            return 1


def main():
    """Entry point for CLI."""
    cli = CLI()
    sys.exit(cli.run())
