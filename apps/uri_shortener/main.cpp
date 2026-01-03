#include "UriShortenerBuilder.h"

#include <Log.h>

int main() {
  auto result = uri_shortener::UriShortenerBuilder::bootstrap();
  if (result.is_err()) {
    obs::error("Failed to start URI Shortener",
               {{"error", uri_shortener::to_string(result.error())}});
    return 1;
  }
  return result.value().run();
}
