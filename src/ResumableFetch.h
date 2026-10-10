#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "SecureHttpClient.h"

// Simulator stand-in for the SDK's ResumableFetch.h: same types and entry
// point, but one plain GET per call. The stub HTTP client has no Range or
// redirect handling, so a transfer either completes or fails whole.
namespace freeink {

struct FetchOptions {
  size_t startOffset = 0;
  bool redirectToHttp = false;
};

struct FetchSink {
  std::function<bool(const uint8_t *data, size_t len)> write;
  std::function<bool()> rewind;
  std::function<void(size_t bytes, size_t total)> progress;
};

struct FetchResult {
  int status = 0;
  size_t bytes = 0;
  size_t total = 0;
  bool complete = false;
  bool stopped = false;
  bool aborted = false;
};

inline std::string fetchOrigin(const std::string &url) {
  const size_t schemeEnd = url.find("://");
  if (schemeEnd == std::string::npos) return {};
  const size_t hostStart = schemeEnd + 3;
  const size_t hostEnd = url.find_first_of("/?#", hostStart);
  return url.substr(0, hostEnd);
}

inline FetchResult fetchResumable(const std::string &startUrl, const FetchOptions &options,
                                  const std::function<void(SecureHttpClient &, bool sameOrigin)> &configure,
                                  const FetchSink &sink, const SecureHttpClient::AbortCallback &shouldAbort = nullptr) {
  FetchResult result;
  SecureHttpClient http;
  if (!http.begin(startUrl)) {
    result.status = -1;
    return result;
  }
  if (configure) configure(http, true);
  // A resume start offset is dropped: the whole body is fetched again from byte 0.
  if (options.startOffset != 0 && sink.rewind && !sink.rewind()) {
    result.stopped = true;
    return result;
  }
  result.status = http.sendRequest(
      "GET", nullptr, 0,
      [&](const uint8_t *data, size_t len) {
        if (sink.write && !sink.write(data, len)) {
          result.stopped = true;
          return false;
        }
        result.bytes += len;
        if (sink.progress) sink.progress(result.bytes, result.total);
        return true;
      },
      shouldAbort);
  if (shouldAbort && shouldAbort()) result.aborted = true;
  if (result.status >= 200 && result.status < 300 && !result.stopped && !result.aborted) {
    result.total = result.bytes;
    result.complete = http.responseComplete();
  }
  http.end();
  return result;
}

}  // namespace freeink
