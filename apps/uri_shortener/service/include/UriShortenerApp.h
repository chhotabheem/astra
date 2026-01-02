#pragma once

#include "UriShortenerComponents.h"

namespace uri_shortener {

class UriShortenerApp {
public:
  explicit UriShortenerApp(UriShortenerComponents components);

  [[nodiscard]] int run();

  UriShortenerApp(UriShortenerApp &&) noexcept;
  UriShortenerApp &operator=(UriShortenerApp &&) noexcept;
  ~UriShortenerApp();

  UriShortenerApp(const UriShortenerApp &) = delete;
  UriShortenerApp &operator=(const UriShortenerApp &) = delete;

private:
  UriShortenerComponents m_components;
};

} // namespace uri_shortener
