"""
Configuration management module.
Handles loading and validation of configuration from JSON files.
"""

import json
from pathlib import Path
from typing import Dict, Optional


class ConfigurationError(Exception):
    """Raised when configuration is invalid or missing."""
    pass


class ConfigManager:
    """Manages application configuration."""
    
    REQUIRED_FIELDS = ['image_name', 'container_name', 'container_workdir']
    DEFAULT_VALUES = {
        'network_mode': 'host',
        'log_level': 'INFO',
        'dockerfile_path': '.'
    }
    
    def __init__(self, config_path: Optional[Path] = None):
        """
        Initialize configuration manager.
        
        Args:
            config_path: Path to configuration file
            
        Raises:
            ConfigurationError: If configuration is invalid
        """
        if config_path is None:
            config_path = Path(__file__).parent.parent / "config.json"
        
        self.config_path = Path(config_path)
        self.config = self._load_config()
    
    def _load_config(self) -> Dict:
        """Load and validate configuration from JSON file."""
        try:
            with open(self.config_path, 'r') as f:
                config = json.load(f)
        except FileNotFoundError:
            raise ConfigurationError(f"Configuration file not found: {self.config_path}")
        except json.JSONDecodeError as e:
            raise ConfigurationError(f"Invalid JSON in configuration file: {e}")
        
        self._validate_config(config)
        self._apply_defaults(config)
        
        return config
    
    def _validate_config(self, config: Dict) -> None:
        """Validate required configuration fields."""
        missing_fields = [field for field in self.REQUIRED_FIELDS if field not in config]
        
        if missing_fields:
            raise ConfigurationError(
                f"Missing required configuration fields: {', '.join(missing_fields)}"
            )
    
    def _apply_defaults(self, config: Dict) -> None:
        """Apply default values for optional fields."""
        for key, value in self.DEFAULT_VALUES.items():
            config.setdefault(key, value)
    
    def get(self, key: str, default=None):
        """Get configuration value by key."""
        return self.config.get(key, default)
    
    def __getitem__(self, key: str):
        """Get configuration value using dictionary syntax."""
        return self.config[key]
    
    def __contains__(self, key: str) -> bool:
        """Check if configuration key exists."""
        return key in self.config
