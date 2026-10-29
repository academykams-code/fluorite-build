//
//  guest_picker.mm
//  Reconstructed from libloader (Objective-C Implementation)
//
//  UIDocumentPickerDelegate for 8 Ball Pool Guest Token Import / Export
//

#import "guest_picker.h"

@implementation FluoriteGuestPickerDelegate

- (instancetype)initWithHandler:(nullable FluoriteGuestPickerHandler)handler {
    self = [super init];
    if (self) {
        _handler = [handler copy];
    }
    return self;
}

#pragma mark - UIDocumentPickerDelegate

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    if (urls.count > 0 && _handler) {
        NSURL *selectedUrl = urls.firstObject;
        _handler(selectedUrl, nil);
    }
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
    if (_handler) {
        NSError *cancelError = [NSError errorWithDomain:@"com.fluorite.picker"
                                                   code:-1
                                               userInfo:@{NSLocalizedDescriptionKey: @"Document picker was cancelled"}];
        _handler(nil, cancelError);
    }
}

@end
