#pragma once

#include "Result.h"
#include "UriShortenerApp.h"
#include "UriShortenerComponents.h"
#include "uri_shortener.pb.h"

#include <string>

namespace uri_shortener {

enum class BuilderError { InvalidConfig, ServerCreationFailed };

inline std::string to_string(BuilderError err) {
  switch (err) {
  case BuilderError::InvalidConfig:
    return "InvalidConfig";
  case BuilderError::ServerCreationFailed:
    return "ServerCreationFailed";
  }
  return "Unknown";
}

class UriShortenerBuilder {
public:
  static astra::outcome::Result<UriShortenerApp, BuilderError> bootstrap();

  explicit UriShortenerBuilder(const Config &config);

  UriShortenerBuilder &domain();
  UriShortenerBuilder &backend();
  UriShortenerBuilder &messaging();
  UriShortenerBuilder &resilience();

  astra::outcome::Result<UriShortenerApp, BuilderError> build();

private:
  UriShortenerBuilder &repo();
  UriShortenerBuilder &codeGen();
  UriShortenerBuilder &useCases();

  UriShortenerBuilder &httpClient();
  UriShortenerBuilder &serviceResolver();
  UriShortenerBuilder &dataAdapter();

  UriShortenerBuilder &msgHandler();
  UriShortenerBuilder &executor();
  UriShortenerBuilder &reqHandler();
  UriShortenerBuilder &wrapObservable();

  UriShortenerBuilder &loadShedder();

  void initObservability();

  const Config &m_config;
  UriShortenerComponents m_components;
};

} // namespace uri_shortener
