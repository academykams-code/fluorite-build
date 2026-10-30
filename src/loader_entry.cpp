//
//  loader_entry.cpp
//  Reconstructed from libloader (__mod_init_func & Entry Chains)
//
//  Framework Constructor & Dylib Initialization Routine
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include "resolver.h"
#include "anti_tamper.h"
#include "remote_attestation.h"
#include "game_rpc_handler.h"
#include "user_interface_view.h"
#include "payload_loader.h"
#include <cstdio>

static UserInterfaceView *g_overlayView = nil;
static bool g_initialized = false;

static UIWindow* GetActiveKeyWindow() {
    UIWindow *keyWindow = nil;
    if (@available(iOS 13.0, *)) {
        for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
            if (scene.activationState == UISceneActivationStateForegroundActive &&
                [scene isKindOfClass:[UIWindowScene class]]) {
                UIWindowScene *windowScene = (UIWindowScene *)scene;
                for (UIWindow *w in windowScene.windows) {
                    if (w.isKeyWindow) {
                        keyWindow = w;
                        break;
                    }
                }
                if (!keyWindow && windowScene.windows.count > 0) {
                    keyWindow = windowScene.windows.firstObject;
                }
                if (keyWindow) break;
            }
        }
    }
    if (!keyWindow) {
        keyWindow = [UIApplication sharedApplication].keyWindow;
    }
    if (!keyWindow && [UIApplication sharedApplication].windows.count > 0) {
        keyWindow = [UIApplication sharedApplication].windows.firstObject;
    }
    return keyWindow;
}

static void InitializeFluoriteFramework() {
    if (g_initialized) return;
    g_initialized = true;

    printf("====================================================\n");
    printf("  Fluorite Loader (fluo_8ballpool v2.6.2) Initializing\n");
    printf("  Target: 8 Ball Pool v56.31.0\n");
    printf("====================================================\n");

    @try {
        // 1. Locate and unpack fluorite_resources.tar.lz4 (extracts fs/x_data.bin payload)
        loader::PayloadLoader& payloadLoader = loader::PayloadLoader::Instance();
        std::string archivePath = payloadLoader.GetResourceArchivePath();
        if (!archivePath.empty()) {
            printf("[Loader] Found resource archive: %s\n", archivePath.c_str());
            std::vector<uint8_t> payload;
            if (payloadLoader.ExtractPayloadToMemory(payload)) {
                printf("[Loader] Successfully mapped payload (size: %zu bytes)\n", payload.size());
            }
        } else {
            printf("[WARN] Resource archive fluorite_resources.tar.lz4 not found in bundle.\n");
        }

        // 2. Anti-debugging / integrity checks
        security::AntiTamper& antiTamper = security::AntiTamper::Instance();
        if (!antiTamper.VerifyBinaryIntegrity()) {
            printf("[FATAL] Binary integrity verification failed!\n");
            return;
        }

        if (antiTamper.DetectAnalysisTools()) {
            printf("[WARN] Reverse engineering / instrumentation framework active.\n");
        }

        // 3. Install ad suppression hooks
        antiTamper.InstallAdBlockHooks();

        // 4. Initialize dynamic Cocos2d-x ivar & symbol resolver
        pool::Resolver::Instance().Initialize(0);

        // 5. Initialize JSON-RPC 2.0 dispatcher
        hack::GameRpcHandler::Instance().setup_handlers();

        // 6. Verify saved license with license server (or await user key input)
        fluorite::RemoteAttestation::Instance().Initialize();

        // 7. Inject Metal UI overlay with a short delay to ensure root view controller is ready
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            @try {
                UIWindow *keyWindow = GetActiveKeyWindow();
                if (keyWindow && !g_overlayView) {
                    g_overlayView = [[UserInterfaceView alloc] init:keyWindow];
                    [keyWindow addSubview:g_overlayView];
                    [keyWindow bringSubviewToFront:g_overlayView];
                    printf("[UI] Successfully injected Metal UserInterfaceView into key window.\n");
                } else {
                    printf("[UI] Key window not available yet or overlay already exists.\n");
                }
            } @catch (NSException *ex) {
                printf("[UI] Exception during overlay injection: %s\n", [ex.reason UTF8String]);
            }
        });
    } @catch (NSException *exception) {
        printf("[FATAL] Exception during Fluorite initialization: %s\n", [exception.reason UTF8String]);
    }
}

// Mach-O dynamic library initializer constructor attribute
__attribute__((constructor))
static void libloader_dylib_entry() {
    // 1. Observe application launch completion
    [[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationDidFinishLaunchingNotification
                                                      object:nil
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification * _Nonnull note) {
        InitializeFluoriteFramework();
    }];

    // 2. Fallback: If notification was already sent or custom run loop is used, initialize after 2 seconds
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        if (!g_initialized) {
            printf("[Loader] Triggering initialization via fallback timer...\n");
            InitializeFluoriteFramework();
        }
    });
}
