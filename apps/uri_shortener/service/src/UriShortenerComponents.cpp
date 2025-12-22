#include "UriShortenerComponents.h"

// Include complete type definitions for unique_ptr members
#include "Http2Client.h"
#include "Http2Server.h"
#include "IServiceResolver.h"
#include "StickyQueue.h"
#include "AtomicLoadShedder.h"
#include "UriShortenerMessageHandler.h"
#include "ObservableMessageHandler.h"
#include "UriShortenerRequestHandler.h"
#include "ObservableRequestHandler.h"

namespace uri_shortener {

// These definitions require complete types for unique_ptr members
UriShortenerComponents::UriShortenerComponents() = default;
UriShortenerComponents::~UriShortenerComponents() = default;
UriShortenerComponents::UriShortenerComponents(UriShortenerComponents&&) = default;
UriShortenerComponents& UriShortenerComponents::operator=(UriShortenerComponents&&) = default;

} // namespace uri_shortener
