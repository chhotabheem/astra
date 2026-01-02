#pragma once

namespace astra::http2 {

enum class Http2ClientError {
  ConnectionFailed,
  RequestTimeout,
  StreamClosed,
  NotConnected,
  SubmitFailed
};

} // namespace astra::http2
