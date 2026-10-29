//
//  guest_picker.h
//  Reconstructed from libloader (Objective-C Runtime Metadata)
//
//  UIDocumentPickerDelegate for 8 Ball Pool Guest Token Import / Export
//

#pragma once

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^FluoriteGuestPickerHandler)(NSURL * _Nullable url, NSError * _Nullable error);

@interface FluoriteGuestPickerDelegate : NSObject <UIDocumentPickerDelegate>

@property (nonatomic, copy, nullable) FluoriteGuestPickerHandler handler;

- (instancetype)initWithHandler:(nullable FluoriteGuestPickerHandler)handler;

@end

NS_ASSUME_NONNULL_END
