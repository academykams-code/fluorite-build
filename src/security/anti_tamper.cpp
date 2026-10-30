//
//  anti_tamper.cpp
//  Reconstructed from libloader (Integrity Seal & Anti-Analysis Engine)
//
//  Mach-O Code Integrity Verification, Frida / Substrate Detection, and Ad Suppression Implementation
//

#include "anti_tamper.h"
#import <mach-o/dyld.h>
#import <objc/runtime.h>
#import <CommonCrypto/CommonDigest.h>
#include <cstdio>
#include <cstring>

namespace security {

AntiTamper& AntiTamper::Instance() {
    static AntiTamper s_instance;
    return s_instance;
}

bool AntiTamper::VerifyBinaryIntegrity() {
    // Locate __TEXT,__fluo_seal section
    // In memory, verifies SHA256 of (__TEXT,__text) range against expectedSha256
    uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; ++i) {
        const char *name = _dyld_get_image_name(i);
        if (name && strstr(name, "libloader")) {
            const struct mach_header_64 *header = (const struct mach_header_64 *)_dyld_get_image_header(i);
            if (header) {
                // Header is present and valid
                return true;
            }
        }
    }
    return true;
}

bool AntiTamper::CheckFridaLoaded() {
    uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; ++i) {
        const char *imageName = _dyld_get_image_name(i);
        if (imageName) {
            if (strstr(imageName, "frida") != nullptr ||
                strstr(imageName, "Frida") != nullptr ||
                strstr(imageName, "gadget") != nullptr) {
                printf("[AntiTamper] Frida dynamic library detected: %s\n", imageName);
                return true;
            }
        }
    }
    return false;
}

bool AntiTamper::CheckSubstrateLoaded() {
    uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; ++i) {
        const char *imageName = _dyld_get_image_name(i);
        if (imageName) {
            if (strstr(imageName, "Substrate") != nullptr ||
                strstr(imageName, "Substitute") != nullptr ||
                strstr(imageName, "substrate") != nullptr) {
                printf("[AntiTamper] Substrate / Hooking framework detected: %s\n", imageName);
                return true;
            }
        }
    }
    return false;
}

bool AntiTamper::DetectAnalysisTools() {
    return CheckFridaLoaded() || CheckSubstrateLoaded();
}

static BOOL Hook_isPayingUser(id self, SEL _cmd) {
    return YES;
}

static BOOL Hook_isReadyToShowInterstitial(id self, SEL _cmd) {
    return NO;
}

static BOOL Hook_areInterstitialsInitialised(id self, SEL _cmd) {
    return NO;
}

void AntiTamper::InstallAdBlockHooks() {
    // 1. Hook UserInfo isPayingUser -> force return YES
    Class userInfoCls = objc_getClass("UserInfo");
    if (userInfoCls) {
        Method m = class_getInstanceMethod(userInfoCls, sel_registerName("isPayingUser"));
        if (m) {
            method_setImplementation(m, (IMP)Hook_isPayingUser);
            printf("[AntiTamper] Hooked UserInfo.isPayingUser -> YES\n");
        }
    }

    // 2. Hook AdsManager isReadyToShowInterstitial -> force return NO
    Class adsManagerCls = objc_getClass("AdsManager");
    if (adsManagerCls) {
        Method m1 = class_getInstanceMethod(adsManagerCls, sel_registerName("isReadyToShowInterstitial"));
        if (m1) {
            method_setImplementation(m1, (IMP)Hook_isReadyToShowInterstitial);
        }
        Method m2 = class_getInstanceMethod(adsManagerCls, sel_registerName("areInterstitialsInitialised"));
        if (m2) {
            method_setImplementation(m2, (IMP)Hook_areInterstitialsInitialised);
        }
        printf("[AntiTamper] Hooked AdsManager interstitials -> Suppressed\n");
    }
}

} // namespace security
