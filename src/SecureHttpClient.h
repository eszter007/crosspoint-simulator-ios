#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "HTTPClient.h"
#include "NetworkClient.h"
#include "WString.h"

namespace freeink {

class SecureHttpClient {
public:
  using DataCallback = std::function<bool(const uint8_t *, size_t)>;
  using AbortCallback = std::function<bool()>;

  void setCACert(const char *) {}
  void setInsecure() {}
  void setUserAgent(const std::string &ua) { http_.addHeader("User-Agent", ua.c_str()); }

  bool begin(const String &url) {
    http_.begin(client_, url.c_str());
    return !url.isEmpty();
  }

  bool begin(const std::string &url) { return begin(String(url)); }
  bool begin(const char *url) { return begin(String(url)); }

  void end() { http_.end(); }

  void addHeader(const char *name, const String &value) {
    http_.addHeader(name, value);
  }

  void addHeader(const char *name, const std::string &value) {
    http_.addHeader(name, value.c_str());
  }

  void addHeader(const char *name, const char *value) {
    http_.addHeader(name, value);
  }

  void addHeader(const std::string &name, const std::string &value) {
    http_.addHeader(name.c_str(), value.c_str());
  }

  void setTimeout(uint16_t ms) { http_.setTimeout(ms); }
  void setReuse(bool reuse) { http_.setReuse(reuse); }

  int GET() {
    status_ = http_.GET();
    return status_;
  }
  // Streaming form: the stub fetches the whole body, then hands it over in one chunk.
  int GET(const DataCallback &onData) {
    status_ = http_.GET();
    if (status_ < 0) return status_;
    const std::string &body = getString();
    if (!body.empty() && onData) onData(reinterpret_cast<const uint8_t *>(body.data()), body.size());
    return status_;
  }
  int getStatus() const { return status_; }
  bool hasContentLength() const { return const_cast<HTTPClient &>(http_).getSize() >= 0; }
  size_t getContentLength() const {
    const int size = const_cast<HTTPClient &>(http_).getSize();
    return size > 0 ? static_cast<size_t>(size) : 0;
  }
  // Absolute locations as given; a path is resolved against the base's scheme and host.
  static bool resolveUrl(const std::string &baseUrl, const std::string &location, std::string &resolved) {
    if (location.find("://") != std::string::npos) {
      resolved = location;
      return true;
    }
    const size_t schemeEnd = baseUrl.find("://");
    if (schemeEnd == std::string::npos) return false;
    const size_t pathStart = baseUrl.find('/', schemeEnd + 3);
    const std::string origin = pathStart == std::string::npos ? baseUrl : baseUrl.substr(0, pathStart);
    if (!location.empty() && location[0] == '/') {
      resolved = origin + location;
      return true;
    }
    const std::string path = pathStart == std::string::npos ? "/" : baseUrl.substr(pathStart);
    resolved = origin + path.substr(0, path.rfind('/') + 1) + location;
    return true;
  }
  int POST(const String &payload) { return http_.POST(payload.c_str()); }
  // The SDK declares POST on std::string; begin(), addHeader() and
  // sendRequest() already carry the matching overload and this one was missed.
  int POST(const std::string &payload) { return http_.POST(payload.c_str()); }
  int sendRequest(const char *method, const String &payload) {
    if (method && std::string(method) == "PUT") {
      return http_.PUT(payload);
    }
    if (method && std::string(method) == "POST") {
      return http_.POST(payload.c_str());
    }
    return http_.GET();
  }
  int sendRequest(const char *method, const std::string &payload) {
    return sendRequest(method, String(payload));
  }
  int sendRequest(const char *method, const uint8_t *payload, size_t payloadLen) {
    return sendRequest(method, String(std::string(reinterpret_cast<const char *>(payload), payloadLen)));
  }
  // Streaming form: the stub fetches the whole body, then hands it over in one chunk.
  int sendRequest(const char *method, const uint8_t *payload, size_t payloadLen, const DataCallback &onData,
                  const AbortCallback &shouldAbort = nullptr) {
    responseComplete_ = false;
    const int status = sendRequest(method, payload, payloadLen);
    if (status < 0) return status;
    if (shouldAbort && shouldAbort()) return status;
    const std::string &body = getString();
    if (!body.empty() && onData &&
        !onData(reinterpret_cast<const uint8_t *>(body.data()), body.size()))
      return status;
    responseComplete_ = true;
    return status;
  }
  bool responseComplete() const { return responseComplete_; }
  // SDK setters with no behaviour here: the stub follows the host's defaults.
  void setBasicAuth(const std::string &user, const std::string &pass) { http_.setAuthorization(user.c_str(), pass.c_str()); }
  void clearBasicAuth() {}
  void setFollowRedirects(int) {}
  void setAllowRedirectDowngrade(bool) {}
  std::string getHeader(const std::string &) const { return {}; }
  bool callbackAborted() const { return false; }
  bool aborted() const { return false; }
  std::vector<std::pair<std::string, std::string>> getHeaders() const { return {}; }

  // The SDK returns `const std::string&`, and firmware binds the result
  // straight to a std::string. Cached in a member because the Arduino-style
  // client hands back a String by value, and a reference to that temporary
  // would dangle.
  const std::string &getString() {
    // .s, not .c_str(): a binary body (a .cplang pack, an image) holds zero bytes.
    body_ = http_.getString().s;
    return body_;
  }
  int getSize() { return http_.getSize(); }

  static bool tls13Available() { return true; }

private:
  NetworkClientSecure client_;
  HTTPClient http_;
  std::string body_;
  bool responseComplete_ = true;
  int status_ = 0;
};

} // namespace freeink
