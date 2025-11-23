"""
Command-line interface module.
"""

import argparse
import sys
from typing import Optional

from .docker import DockerManager, DockerError


class CLI:
    """Command-line interface for Astra Builder."""
    
    def __init__(self):
        """Initialize CLI."""
        self.parser = self._create_parser()
    
    def _create_parser(self) -> argparse.ArgumentParser:
        """Create argument parser."""
        parser = argparse.ArgumentParser(
            description="Astra Docker Environment Manager - Production Build Tool",
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog="""
Examples:
  %(prog)s --build                          Build the nghttp2 Docker image
  %(prog)s --start                          Start the container
  %(prog)s --up                             Build image and start container
  %(prog)s --stop                           Stop and remove container
  %(prog)s --build --image astrabuilder:dev Build with custom tag
  %(prog)s --config custom.json             Use custom configuration file
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
        parser.add_argument('--image', type=str, default='astrabuilder:nghttp2',
                           help='Docker image tag (default: astrabuilder:nghttp2)')
        parser.add_argument('--network', type=str, choices=['host', 'bridge'], default='host',
                           help='Docker network mode for build (default: host)')
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
            docker_manager = DockerManager(config_path=parsed_args.config)
            
            # Override config with command-line arguments
            if parsed_args.image:
                docker_manager.config['image_name'] = parsed_args.image
            if parsed_args.network:
                docker_manager.config['network_mode'] = parsed_args.network
            
            print("=" * 60)
            print("Astra Builder")
            print(f"Image: {docker_manager['image_name']}")
            print(f"Network: {docker_manager.get('network_mode', 'host')}")
            print("=" * 60)
            
            # Execute requested operations
            if parsed_args.stop:
                docker_manager.stop_container()
            
            if parsed_args.build or parsed_args.up:
                docker_manager.build_image()
            
            if parsed_args.start or parsed_args.up:
                docker_manager.start_container()
            
            return 0
            
        except FileNotFoundError as e:
            print(f"❌ Error: {e}", file=sys.stderr)
            return 1
        except DockerError as e:
            print(f"❌ Docker Error: {e}", file=sys.stderr)
            return 1
        except Exception as e:
            print(f"❌ Unexpected Error: {e}", file=sys.stderr)
            return 1


def main():
    """Entry point for CLI."""
    cli = CLI()
    sys.exit(cli.run())
