// HTTP for the iOS build. The desktop simulator shells out to curl
// (src/SimHttpFetch.h); an iOS app can spawn nothing, so the same request is
// made with NSURLSession and waited for, which is what the firmware's blocking
// HTTP clients expect.
#import <Foundation/Foundation.h>

#include <map>
#include <string>

#include "SimHttpFetch.h"

namespace sim_http_fetch {

bool fetchNative(const std::string &url, const char *method,
                 const std::map<std::string, std::string> &headers,
                 const std::string &basicAuth, const char *body,
                 Response &out) {
  @autoreleasepool {
    NSURL *nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
    if (!nsUrl)
      return false;
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:nsUrl];
    request.HTTPMethod = [NSString stringWithUTF8String:(method && *method) ? method : "GET"];
    request.timeoutInterval = 60;
    for (const auto &header : headers) {
      [request setValue:[NSString stringWithUTF8String:header.second.c_str()]
          forHTTPHeaderField:[NSString stringWithUTF8String:header.first.c_str()]];
    }
    if (!basicAuth.empty()) {
      NSData *credentials = [NSData dataWithBytes:basicAuth.data() length:basicAuth.size()];
      [request setValue:[@"Basic " stringByAppendingString:[credentials base64EncodedStringWithOptions:0]]
          forHTTPHeaderField:@"Authorization"];
    }
    if (body)
      request.HTTPBody = [NSData dataWithBytes:body length:strlen(body)];

    __block NSData *received = nil;
    __block NSInteger status = 0;
    dispatch_semaphore_t done = dispatch_semaphore_create(0);
    NSURLSession *session =
        [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration ephemeralSessionConfiguration]];
    [[session dataTaskWithRequest:request
                completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
                  if (!error && [response isKindOfClass:[NSHTTPURLResponse class]]) {
                    status = ((NSHTTPURLResponse *)response).statusCode;
                    received = data;
                  }
                  dispatch_semaphore_signal(done);
                }] resume];
    dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
    [session finishTasksAndInvalidate];

    out.statusCode = static_cast<int>(status);
    if (received)
      out.body.assign(static_cast<const char *>(received.bytes), received.length);
    return status > 0;
  }
}

} // namespace sim_http_fetch
