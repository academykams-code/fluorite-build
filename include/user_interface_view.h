//
//  user_interface_view.h
//  Reconstructed from libloader (Objective-C Runtime Metadata)
//
//  MTKView Overlay & Touch Event Interceptor for ImGui UI & Trajectory Visuals
//

#pragma once

#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import "metal_context.h"

NS_ASSUME_NONNULL_BEGIN

@interface UserInterfaceView : UIView <MTKViewDelegate>

@property (nonatomic, strong) MTKView *mtkView;
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) MetalContext *metalContext;
@property (nonatomic, assign) BOOL menuVisible;

+ (nullable instancetype)sharedOverlay;

- (instancetype)init:(id)parentView;
- (void)onMenuGesture;
- (void)updateIOWithTouchEvent:(UIEvent *)event;
- (void)presentLicenseKeyPrompt;

@end

NS_ASSUME_NONNULL_END
