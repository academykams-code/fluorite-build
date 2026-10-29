//
//  user_interface_view.mm
//  Reconstructed from libloader (Objective-C Implementation)
//
//  MTKView Overlay & Touch Event Interceptor Implementation
//

#import "user_interface_view.h"
#import "remote_attestation.h"
#include <cstdio>

static __weak UserInterfaceView *s_sharedOverlay = nil;

// Forward declaration of internal C++ cheat state & solver
namespace hack {
    static bool s_menuOpen = false;
    bool IsMenuOpen() { return s_menuOpen; }
    void ToggleMenu() { s_menuOpen = !s_menuOpen; }
    void RenderTrajectoryGuidelines() {
        // Trajectory prediction visuals
    }

    void RenderCheatMenu() {
        // Dear ImGui Menu integration
        // Displays license status and provides button to enter/activate key
        auto& auth = fluorite::RemoteAttestation::Instance();
        if (!auth.IsLicenseValid()) {
            printf("[UI Menu] Status: Unlicensed (%s)\n", auth.Status().message.c_str());
        } else {
            printf("[UI Menu] Status: Active License (%s)\n", auth.Status().key.c_str());
        }
    }
}

@implementation UserInterfaceView

+ (nullable instancetype)sharedOverlay {
    return s_sharedOverlay;
}

- (instancetype)init:(id)parentView {
    CGRect frame = [UIScreen mainScreen].bounds;
    self = [super initWithFrame:frame];
    if (self) {
        s_sharedOverlay = self;
        self.backgroundColor = [UIColor clearColor];
        self.userInteractionEnabled = YES;
        self.multipleTouchEnabled = YES;
        _menuVisible = NO;

        // Initialize Metal device and command queue
        _device = MTLCreateSystemDefaultDevice();
        if (_device) {
            _commandQueue = [_device newCommandQueue];
            _metalContext = [[MetalContext alloc] init];
            [_metalContext makeDeviceObjectsWithDevice:_device];

            // Setup MTKView
            _mtkView = [[MTKView alloc] initWithFrame:frame device:_device];
            _mtkView.delegate = self;
            _mtkView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
            _mtkView.backgroundColor = [UIColor clearColor];
            _mtkView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
            _mtkView.sampleCount = 1;
            _mtkView.preferredFramesPerSecond = 60;
            _mtkView.userInteractionEnabled = NO; // Pass touches through to container view
            [self addSubview:_mtkView];
        }

        // Setup triple-tap or three-finger tap gesture for toggling menu
        UITapGestureRecognizer *menuGesture = [[UITapGestureRecognizer alloc] initWithTarget:self
                                                                                      action:@selector(onMenuGesture)];
        menuGesture.numberOfTapsRequired = 3;
        menuGesture.numberOfTouchesRequired = 2; // Two-finger triple tap
        [self addGestureRecognizer:menuGesture];
    }
    return self;
}

- (void)onMenuGesture {
    _menuVisible = !_menuVisible;
    hack::ToggleMenu();

    // If opening menu while unlicensed, optionally prompt user to enter key
    if (_menuVisible && !fluorite::RemoteAttestation::Instance().IsLicenseValid()) {
        [self presentLicenseKeyPrompt];
    }
}

- (void)presentLicenseKeyPrompt {
    dispatch_async(dispatch_get_main_queue(), ^{
        std::string currentKey = fluorite::RemoteAttestation::Instance().GetSavedLicenseKey();
        NSString *currentNsKey = [NSString stringWithUTF8String:currentKey.c_str()];

        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Fluorite License Activation"
                                                                       message:@"Enter your license key to activate features:"
                                                                preferredStyle:UIAlertControllerStyleAlert];

        [alert addTextFieldWithConfigurationHandler:^(UITextField *textField) {
            textField.placeholder = @"XXXX-XXXX-XXXX-XXXX";
            textField.text = currentNsKey;
            textField.autocorrectionType = UITextAutocorrectionTypeNo;
            textField.autocapitalizationType = UITextAutocapitalizationTypeAllCharacters;
            textField.clearButtonMode = UITextFieldViewModeWhileEditing;
        }];

        UIAlertAction *activateAction = [UIAlertAction actionWithTitle:@"Activate"
                                                                 style:UIAlertActionStyleDefault
                                                               handler:^(UIAlertAction *action) {
            UITextField *field = alert.textFields.firstObject;
            NSString *enteredKey = [field.text stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
            if (enteredKey.length > 0) {
                std::string keyStr = [enteredKey UTF8String];
                fluorite::RemoteAttestation::Instance().VerifyLicense(keyStr, ^(const fluorite::LicenseStatus& status) {
                    NSString *title = status.isValid ? @"Activation Succeeded" : @"Activation Failed";
                    NSString *statusMsg = [NSString stringWithUTF8String:status.message.c_str()];

                    UIAlertController *resultAlert = [UIAlertController alertControllerWithTitle:title
                                                                                         message:statusMsg
                                                                                  preferredStyle:UIAlertControllerStyleAlert];
                    [resultAlert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];

                    UIViewController *rootVC = [UIApplication sharedApplication].keyWindow.rootViewController;
                    if (rootVC) {
                        [rootVC presentViewController:resultAlert animated:YES completion:nil];
                    }
                });
            }
        }];

        UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil];

        [alert addAction:activateAction];
        [alert addAction:cancelAction];

        UIViewController *rootVC = [UIApplication sharedApplication].keyWindow.rootViewController;
        if (rootVC) {
            [rootVC presentViewController:alert animated:YES completion:nil];
        }
    });
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    // If menu is open and touch lands within menu bounds, capture it.
    // Otherwise, return nil so touch falls through to 8 Ball Pool Cocos2d-x view.
    if (_menuVisible) {
        return [super hitTest:point withEvent:event];
    }
    return nil;
}

- (void)updateIOWithTouchEvent:(UIEvent *)event {
    NSSet<UITouch *> *touches = [event allTouches];
    UITouch *touch = [touches anyObject];
    if (!touch) return;

    CGPoint loc = [touch locationInView:self];
    (void)loc;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

#pragma mark - MTKViewDelegate

- (void)drawInMTKView:(MTKView *)view {
    // Render frame
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    MTLRenderPassDescriptor *renderPassDesc = view.currentRenderPassDescriptor;
    if (renderPassDesc && commandBuffer) {
        _metalContext.framebufferDescriptor = [[FramebufferDescriptor alloc] initWithRenderPassDescriptor:renderPassDesc];

        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];

        // 1. Render aim guidelines & trajectory overlays (only if licensed)
        if (fluorite::RemoteAttestation::Instance().IsLicenseValid()) {
            hack::RenderTrajectoryGuidelines();
        }

        // 2. Render Dear ImGui menu if toggled on
        if (_menuVisible) {
            hack::RenderCheatMenu();
        }

        [encoder endEncoding];
        id<CAMetalDrawable> drawable = view.currentDrawable;
        if (drawable) {
            [commandBuffer presentDrawable:drawable];
        }
        [commandBuffer commit];
    }
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Resize viewport
}

@end
