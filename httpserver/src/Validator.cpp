#include "Validator.h"
#ifdef HAS_SIMDJSON
#include <simdjson.h>
#endif

namespace httpserver {

bool Validator::quickValidate(const std::string& jsonBody, std::string& error) {
#ifdef HAS_SIMDJSON
    // Use simdjson for fast validation
    static simdjson::dom::parser parser; // Thread-local or static if thread-safe (parser is not thread-safe, need thread_local)
    // Actually, creating parser is cheap-ish but reusing is better.
    // For now, local instance.
    simdjson::dom::parser localParser;
    simdjson::dom::element doc;
    auto err = localParser.parse(jsonBody).get(doc);
    if (err) {
        error = simdjson::error_message(err);
        return false;
    }
    return true;
#else
    // Fallback: simple check
    if (jsonBody.empty()) {
        error = "Empty body";
        return false;
    }
    // Very naive check
    size_t start = jsonBody.find_first_not_of(" \t\n\r");
    if (start == std::string::npos || (jsonBody[start] != '{' && jsonBody[start] != '[')) {
        error = "Invalid JSON format";
        return false;
    }
    return true;
#endif
}

bool Validator::validateSchema(const std::string& jsonBody, const std::string& schemaPath, std::string& error) {
    // TODO: Implement valijson logic when available
    return true;
}

} // namespace httpserver
