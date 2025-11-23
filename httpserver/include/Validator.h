#pragma once

#include <string>

namespace httpserver {

class Validator {
public:
    /**
     * @brief Perform quick validation on the JSON body.
     * 
     * Checks if the body is valid JSON and contains critical fields.
     * Uses simdjson for high performance (when enabled).
     * 
     * @param jsonBody The raw JSON string
     * @param error Output parameter for error message
     * @return true if valid, false otherwise
     */
    static bool quickValidate(const std::string& jsonBody, std::string& error);

    /**
     * @brief Perform full schema validation.
     * 
     * Validates the JSON body against a JSON schema file.
     * Uses valijson (when enabled).
     * 
     * @param jsonBody The raw JSON string
     * @param schemaPath Path to the JSON schema file
     * @param error Output parameter for error message
     * @return true if valid, false otherwise
     */
    static bool validateSchema(const std::string& jsonBody, const std::string& schemaPath, std::string& error);
};

} // namespace httpserver
