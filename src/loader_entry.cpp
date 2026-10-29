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

static void InitializeFluoriteFramework() {
    printf("====================================================\n");
    printf("  Fluorite Loader (fluo_8ballpool v2.6.2) Initializing\n");
    printf("  Target: 8 Ball Pool v56.31.0\n");
    printf("====================================================\n");

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

    // 2. Verify binary integrity & anti-debugging checks
    security::AntiTamper& antiTamper = security::AntiTamper::Instance();
    if (!antiTamper.VerifyBinaryIntegrity()) {
        printf("[FATAL] Binary integrity verification failed (__fluo_seal mismatch)!\n");
        return;
    }

    if (antiTamper.DetectAnalysisTools()) {
        printf("[WARN] Reverse engineering / instrumentation framework active.\n");
    }

    // 2. Install ad suppression hooks
    antiTamper.InstallAdBlockHooks();

    // 3. Initialize dynamic Cocos2d-x ivar & symbol resolver
    pool::Resolver::Instance().Initialize(0);

    // 4. Initialize JSON-RPC 2.0 dispatcher
    hack::GameRpcHandler::Instance().setup_handlers();

    // 5. Verify saved license with license server (or await user key input)
    fluorite::RemoteAttestation::Instance().Initialize();

    // 6. Hook application window presentation to inject Metal UI overlay
    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow *keyWindow = [UIApplication sharedApplication].keyWindow;
        if (!keyWindow && [UIApplication sharedApplication].windows.count > 0) {
            keyWindow = [UIApplication sharedApplication].windows.firstObject;
        }

        if (keyWindow && !g_overlayView) {
            g_overlayView = [[UserInterfaceView alloc] init:keyWindow];
            [keyWindow addSubview:g_overlayView];
            [keyWindow bringSubviewToFront:g_overlayView];
            printf("[UI] Successfully injected Metal UserInterfaceView into key window.\n");
        }
    });
}

// Mach-O dynamic library initializer constructor attribute
__attribute__((constructor))
static void libloader_dylib_entry() {
    InitializeFluoriteFramework();
}
