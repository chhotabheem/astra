#include "UriShortenerComponents.h"

// Include complete type definitions for unique_ptr members
#include "AffinityExecutor.h"
#include "AtomicLoadShedder.h"
#include "Http2Client.h"
#include "Http2Server.h"
#include "IServiceResolver.h"
#include "ObservableMessageHandler.h"
#include "ObservableRequestHandler.h"
#include "UriShortenerMessageHandler.h"
#include "UriShortenerRequestHandler.h"

namespace uri_shortener {

// These definitions require complete types for unique_ptr members
UriShortenerComponents::UriShortenerComponents() = default;
UriShortenerComponents::~UriShortenerComponents() = default;
UriShortenerComponents::UriShortenerComponents(UriShortenerComponents &&) =
    default;
UriShortenerComponents &
UriShortenerComponents::operator=(UriShortenerComponents &&) = default;

} // namespace uri_shortener
