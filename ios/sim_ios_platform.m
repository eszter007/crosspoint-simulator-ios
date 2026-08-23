// iOS platform glue for the CrossPoint simulator.
//
// The simulator's SD card is a host directory. iOS has no working directory to
// speak of, so the card is either:
//
//   * the app's own Documents directory (the default), which Info.plist marks
//     file-shareable so it shows up in the Files app and over USB; or
//   * any folder the user picks with the system folder picker -- an iCloud
//     Drive folder, a USB drive mounted in Files, another app's shared folder.
//
// A picked folder is remembered as a security-scoped bookmark, so the choice
// survives relaunches without re-prompting.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "SimulatorPlatform.h"

static NSString *const kBookmarkDefaultsKey = @"CrossPointSimulatorLibraryBookmark";

// Resolved root, held for the process lifetime. simPlatformDocumentsPath() is
// called on every storage path resolution, so it must not do real work per call.
static char gRootPath[1024];
// The URL whose security scope is currently open. Access is never stopped: the
// folder is in use for as long as the app runs.
static NSURL *gScopedURL = nil;

static NSString *documentsDirectory(void) {
  NSArray<NSString *> *paths =
      NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  return paths.firstObject;
}

// Reopen the folder chosen on a previous run. Returns nil when there is no
// bookmark, or when it no longer resolves -- a folder can be deleted, or live
// on a drive that is no longer attached.
static NSURL *resolveStoredBookmark(void) {
  NSData *bookmark = [NSUserDefaults.standardUserDefaults dataForKey:kBookmarkDefaultsKey];
  if (bookmark == nil) {
    return nil;
  }
  BOOL stale = NO;
  NSError *error = nil;
  NSURL *url = [NSURL URLByResolvingBookmarkData:bookmark
                                         options:NSURLBookmarkResolutionWithoutUI
                                   relativeToURL:nil
                             bookmarkDataIsStale:&stale
                                           error:&error];
  if (url == nil) {
    NSLog(@"[SIM] stored library folder could not be resolved: %@", error);
    [NSUserDefaults.standardUserDefaults removeObjectForKey:kBookmarkDefaultsKey];
    return nil;
  }
  if (![url startAccessingSecurityScopedResource]) {
    NSLog(@"[SIM] stored library folder is no longer accessible; falling back to Documents");
    [NSUserDefaults.standardUserDefaults removeObjectForKey:kBookmarkDefaultsKey];
    return nil;
  }
  if (stale) {
    // Refresh it so the next launch resolves without a rebuild.
    NSData *refreshed = [url bookmarkDataWithOptions:NSURLBookmarkCreationMinimalBookmark
                      includingResourceValuesForKeys:nil
                                       relativeToURL:nil
                                               error:NULL];
    if (refreshed != nil) {
      [NSUserDefaults.standardUserDefaults setObject:refreshed forKey:kBookmarkDefaultsKey];
    }
  }
  return url;
}

static void setRootPath(NSString *path) {
  if (path == nil) {
    gRootPath[0] = '\0';
    return;
  }
  const char *utf8 = path.fileSystemRepresentation;
  if (utf8 == NULL) {
    gRootPath[0] = '\0';
    return;
  }
  strlcpy(gRootPath, utf8, sizeof(gRootPath));
}

const char *simPlatformDocumentsPath(void) {
  if (gRootPath[0]) {
    return gRootPath;
  }
  NSURL *stored = resolveStoredBookmark();
  if (stored != nil) {
    gScopedURL = stored;
    setRootPath(stored.path);
  } else {
    setRootPath(documentsDirectory());
  }
  return gRootPath[0] ? gRootPath : NULL;
}

// --- folder picker -----------------------------------------------------------

@interface SimFolderPicker : NSObject <UIDocumentPickerDelegate>
@end

@implementation SimFolderPicker

+ (instancetype)shared {
  static SimFolderPicker *shared = nil;
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    shared = [[SimFolderPicker alloc] init];
  });
  return shared;
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller
    didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
  NSURL *url = urls.firstObject;
  if (url == nil) {
    return;
  }
  if (![url startAccessingSecurityScopedResource]) {
    NSLog(@"[SIM] picked folder could not be opened");
    return;
  }

  NSError *error = nil;
  NSData *bookmark = [url bookmarkDataWithOptions:NSURLBookmarkCreationMinimalBookmark
                   includingResourceValuesForKeys:nil
                                    relativeToURL:nil
                                            error:&error];
  if (bookmark != nil) {
    [NSUserDefaults.standardUserDefaults setObject:bookmark forKey:kBookmarkDefaultsKey];
  } else {
    // Usable for this run; the choice just will not survive a relaunch.
    NSLog(@"[SIM] could not bookmark picked folder: %@", error);
  }

  if (gScopedURL != nil && ![gScopedURL isEqual:url]) {
    [gScopedURL stopAccessingSecurityScopedResource];
  }
  gScopedURL = url;
  setRootPath(url.path);
  NSLog(@"[SIM] library folder set to %@", url.path);
}

@end

static UIViewController *topViewController(void) {
  UIWindow *keyWindow = nil;
  for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
    if (![scene isKindOfClass:UIWindowScene.class]) {
      continue;
    }
    for (UIWindow *window in ((UIWindowScene *)scene).windows) {
      if (window.isKeyWindow) {
        keyWindow = window;
        break;
      }
    }
    if (keyWindow != nil) {
      break;
    }
  }
  UIViewController *vc = keyWindow.rootViewController;
  while (vc.presentedViewController != nil) {
    vc = vc.presentedViewController;
  }
  return vc;
}

bool simPlatformCanPickFolder(void) { return true; }

void simPlatformPickFolder(void) {
  // UIKit is main-thread only. The caller is the SDL event pump, which already
  // runs there, but a dispatch keeps that from being load-bearing.
  dispatch_async(dispatch_get_main_queue(), ^{
    UIViewController *presenter = topViewController();
    if (presenter == nil) {
      NSLog(@"[SIM] no view controller to present the folder picker from");
      return;
    }
    UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc]
        initForOpeningContentTypes:@[ UTTypeFolder ]];
    picker.delegate = SimFolderPicker.shared;
    picker.allowsMultipleSelection = NO;
    [presenter presentViewController:picker animated:YES completion:nil];
  });
}
