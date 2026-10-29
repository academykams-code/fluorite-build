//
//  import_manager.cpp
//  Reconstructed from libloader (C++ RTTI & Account Manager Metadata)
//
//  Guest Token & Account Management Implementation
//

#include "import_manager.h"
#import "guest_picker.h"
#import <UIKit/UIKit.h>

namespace fluorite {

ImportManager& ImportManager::Instance() {
    static ImportManager s_instance;
    return s_instance;
}

std::string ImportManager::GetCurrentGuestToken() {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *token = [defaults stringForKey:@"auth-guest-token"];
    if (!token) {
        // Fallback to guestTokenDomain.plist inside Documents / Library / Preferences
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
        if (paths.count > 0) {
            NSString *prefPath = [paths[0] stringByAppendingPathComponent:@"Preferences/guestTokenDomain.plist"];
            NSDictionary *dict = [NSDictionary dictionaryWithContentsOfFile:prefPath];
            token = dict[@"guestToken"];
        }
    }
    return token ? std::string([token UTF8String]) : "";
}

bool ImportManager::ExportCurrentGuestToken(const std::string& destinationDirectory) {
    std::string token = GetCurrentGuestToken();
    if (token.empty()) {
        return false;
    }

    NSString *destDir = [NSString stringWithUTF8String:destinationDirectory.c_str()];
    NSString *filePath = [destDir stringByAppendingPathComponent:@"guestTokenDomain.plist"];

    NSDictionary *data = @{
        @"guestToken": [NSString stringWithUTF8String:token.c_str()],
        @"domain": @"guestTokenDomain",
        @"exportedAt": [NSDate date]
    };

    return [data writeToFile:filePath atomically:YES];
}

bool ImportManager::ImportGuestTokenFromFile(const std::string& sourcePlistPath) {
    NSString *path = [NSString stringWithUTF8String:sourcePlistPath.c_str()];
    NSDictionary *dict = [NSDictionary dictionaryWithContentsOfFile:path];
    if (!dict || !dict[@"guestToken"]) {
        return false;
    }

    NSString *token = dict[@"guestToken"];
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults setObject:token forKey:@"auth-guest-token"];
    [defaults synchronize];
    return true;
}

void ImportManager::PresentDocumentPickerForImport() {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSArray *types = @[@"com.apple.property-list", @"public.xml", @"public.data"];
        UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:types
                                                                                                        inMode:UIDocumentPickerModeImport];
        FluoriteGuestPickerDelegate *delegate = [[FluoriteGuestPickerDelegate alloc] initWithHandler:^(NSURL * _Nullable url, NSError * _Nullable error) {
            if (url) {
                ImportGuestTokenFromFile(url.path.UTF8String);
            }
        }];
        picker.delegate = delegate;

        UIViewController *rootVC = [UIApplication sharedApplication].keyWindow.rootViewController;
        [rootVC presentViewController:picker animated:YES completion:nil];
    });
}

void ImportManager::PresentDocumentPickerForExport() {
    // Exports token and prompts document picker to save
}

} // namespace fluorite
