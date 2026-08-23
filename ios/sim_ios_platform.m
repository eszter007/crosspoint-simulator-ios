// iOS platform glue for the CrossPoint simulator.
//
// The simulator's SD card is a host directory. iOS gives an app exactly one
// writable place, its Documents directory, so that is where the card lives --
// declared file-shareable in Info.plist so books can be dropped in from the
// Files app, the way they would be copied onto a card.

#import <Foundation/Foundation.h>

#include "SimulatorPlatform.h"

const char *simPlatformDocumentsPath(void) {
  static char cached[1024];
  if (cached[0]) {
    return cached;
  }
  NSArray<NSString *> *paths =
      NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString *documents = paths.firstObject;
  if (documents == nil) {
    return NULL;
  }
  const char *utf8 = documents.UTF8String;
  if (utf8 == NULL) {
    return NULL;
  }
  strlcpy(cached, utf8, sizeof(cached));
  return cached;
}
