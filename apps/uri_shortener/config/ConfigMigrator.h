#pragma once
/// @file ConfigMigrator.h
/// @brief Schema migration following Protobuf industry best practices
/// 
/// Industry Pattern (Google, Kafka, Kubernetes):
/// - Protobuf field numbers are stable forever
/// - Add new fields with defaults (backward compatible)
/// - Mark deleted fields as `reserved` in .proto (never reuse numbers)
/// - Unknown fields are ignored (forward compatible)
/// 
/// This migrator handles explicit transformations when:
/// - Data needs restructuring (field moved between messages)
/// - Values need transformation (e.g., split timeout into request_ms + database_ms)
/// 
/// For simple additions/deprecations, Protobuf handles it automatically.

#include "uri_shortener.pb.h"
#include <string>
#include <optional>
#include <functional>
#include <vector>

namespace uri_shortener {

/// Current schema version the app expects
constexpr int32_t CURRENT_SCHEMA_VERSION = 1;

/// Result of migration attempt
struct MigrationResult {
    bool success{false};
    Config config;
    std::string error;
    int32_t from_version{0};
    int32_t to_version{CURRENT_SCHEMA_VERSION};
    bool migration_applied{false};  // True if actual transformation happened
    
    static MigrationResult ok(Config cfg, int32_t from, bool applied = false) {
        return {true, std::move(cfg), "", from, CURRENT_SCHEMA_VERSION, applied};
    }
    static MigrationResult err(std::string msg, int32_t from) {
        return {false, Config{}, std::move(msg), from, 0, false};
    }
};

/// Migrates config from any version to CURRENT_SCHEMA_VERSION
/// 
/// Following Protobuf best practices:
/// - Backward compatible: New app reads old config (missing fields get defaults)
/// - Forward compatible: Old app reads new config (ignores unknown fields)
/// - Explicit migration only when data transformation is needed
///
class ConfigMigrator {
public:
    /// Migrate config to current version
    /// Applies transformations if version < current
    /// Returns as-is if already current (no transformation needed)
    static MigrationResult migrate(Config config) {
        int32_t source_version = config.schema_version();
        
        // Future version - cannot downgrade, but Protobuf handles unknown fields
        // We accept it but log a warning (caller should handle)
        if (source_version > CURRENT_SCHEMA_VERSION) {
            // Accept future versions - forward compatible
            // Unknown fields are automatically ignored by Protobuf
            return MigrationResult::ok(std::move(config), source_version, false);
        }
        
        // Version 0 or missing - treat as v1 (legacy/default)
        if (source_version < 1) {
            config.set_schema_version(1);
            source_version = 1;
        }
        
        // Already current - no migration needed
        if (source_version == CURRENT_SCHEMA_VERSION) {
            return MigrationResult::ok(std::move(config), source_version, false);
        }
        
        // Apply version-specific transformations
        return applyMigrations(std::move(config), source_version);
    }
    
    /// Check if version needs explicit migration (transformation)
    static bool needsTransformation(int32_t version) {
        // Add versions that need transformation here
        // Currently none - all handled by Protobuf automatically
        return false;
    }
    
    /// Get current schema version
    static constexpr int32_t currentVersion() {
        return CURRENT_SCHEMA_VERSION;
    }

private:
    /// Apply version-specific migrations
    static MigrationResult applyMigrations(Config config, int32_t from_version) {
        // Chain through versions if needed, or direct transform
        // For now, we just set the version (Protobuf handles field defaults)
        
        switch (from_version) {
            // Add explicit transformation cases as schema evolves:
            //
            // case 1:
            //     transformFrom1(config);
            //     [[fallthrough]];  // Continue to next version if chained
            // case 2:
            //     transformFrom2(config);
            //     break;
            
            default:
                // No transformation needed - Protobuf handles defaults
                break;
        }
        
        config.set_schema_version(CURRENT_SCHEMA_VERSION);
        return MigrationResult::ok(std::move(config), from_version, true);
    }
    
    // =========================================================================
    // Version-specific transformations
    // Add functions here when schema changes require data transformation
    // =========================================================================
    
    // Example: v1 → v2 (when we release v2)
    // static void transformFrom1(Config& config) {
    //     // Example: In v1, we had single timeout. In v2, split into request/db.
    //     if (config.has_operational() && config.operational().has_timeouts()) {
    //         auto old_timeout = config.operational().timeouts().request_ms();
    //         if (config.operational().timeouts().database_ms() == 0) {
    //             config.mutable_operational()->mutable_timeouts()->set_database_ms(old_timeout / 2);
    //         }
    //     }
    //     
    //     // Example: In v1, service name was in bootstrap. In v2, also need it in observability.
    //     if (config.has_bootstrap() && config.bootstrap().has_service()) {
    //         auto& svc = config.bootstrap().service();
    //         if (config.mutable_operational()->mutable_observability()->service_version().empty()) {
    //             config.mutable_operational()->mutable_observability()->set_service_version("1.0.0");
    //         }
    //     }
    // }
};

} // namespace uri_shortener
