//
//  remote_attestation.mm
//  Fluorite Reconstructed
//
//  HTTP REST License Server Verification Client Implementation
//

#import "remote_attestation.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include <cstdio>

namespace fluorite {

static NSString *const kLicenseKeyPrefKey = @"FluoriteUserLicenseKey";

RemoteAttestation& RemoteAttestation::Instance() {
    static RemoteAttestation s_instance;
    return s_instance;
}

RemoteAttestation::RemoteAttestation() {
    m_status.key = GetSavedLicenseKey();
}

bool RemoteAttestation::Initialize(const std::string& fallbackKey) {
    std::string key = GetSavedLicenseKey();
    if (key.empty() && !fallbackKey.empty()) {
        key = fallbackKey;
    }

    if (key.empty()) {
        printf("[RemoteAttestation] No saved license key found. Awaiting user input via menu.\n");
        m_status.isValid = false;
        m_status.message = "No license key entered.";
        return false;
    }

    printf("[RemoteAttestation] Verifying license key with server (%s)...\n", m_serverUrl.c_str());
    VerifyLicense(key, [](const LicenseStatus& status) {
        if (status.isValid) {
            printf("[RemoteAttestation] License successfully verified! (Status: %s)\n", status.message.c_str());
        } else {
            printf("[RemoteAttestation] License rejected: %s\n", status.message.c_str());
        }
    });

    return true;
}

std::string RemoteAttestation::GetSavedLicenseKey() const {
    NSString *savedKey = [[NSUserDefaults standardUserDefaults] stringForKey:kLicenseKeyPrefKey];
    return savedKey ? [savedKey UTF8String] : "";
}

void RemoteAttestation::SaveLicenseKey(const std::string& key) {
    NSString *nsKey = [NSString stringWithUTF8String:key.c_str()];
    [[NSUserDefaults standardUserDefaults] setObject:nsKey forKey:kLicenseKeyPrefKey];
    [[NSUserDefaults standardUserDefaults] synchronize];
    m_status.key = key;
}

void RemoteAttestation::VerifySavedLicense(std::function<void(const LicenseStatus&)> callback) {
    std::string key = GetSavedLicenseKey();
    if (key.empty()) {
        m_status.isValid = false;
        m_status.message = "No license key entered.";
        if (callback) callback(m_status);
        return;
    }
    VerifyLicense(key, callback);
}

void RemoteAttestation::VerifyLicense(const std::string& key,
                                     std::function<void(const LicenseStatus&)> callback) {
    if (key.empty()) {
        m_status.isValid = false;
        m_status.message = "License key cannot be empty.";
        if (callback) callback(m_status);
        return;
    }

    NSString *urlString = [NSString stringWithUTF8String:m_serverUrl.c_str()];
    NSURL *url = [NSURL URLWithString:urlString];
    if (!url) {
        m_status.isValid = false;
        m_status.message = "Invalid license server URL configuration.";
        if (callback) callback(m_status);
        return;
    }

    // Build payload: { "license_key": key, "bundle_id": "com.miniclip.8ballpoolmult" }
    NSDictionary *bodyDict = @{
        @"license_key": [NSString stringWithUTF8String:key.c_str()],
        @"bundle_id": @"com.miniclip.8ballpoolmult"
    };

    NSError *jsonError = nil;
    NSData *postData = [NSJSONSerialization dataWithJSONObject:bodyDict options:0 error:&jsonError];
    if (jsonError || !postData) {
        m_status.isValid = false;
        m_status.message = "Failed to serialize JSON license request.";
        if (callback) callback(m_status);
        return;
    }

    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url
                                                           cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                                       timeoutInterval:10.0];
    [request setHTTPMethod:@"POST"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    [request setValue:@"true" forHTTPHeaderField:@"ngrok-skip-browser-warning"];
    [request setHTTPBody:postData];

    NSURLSessionDataTask *task = [[NSURLSession sharedSession] dataTaskWithRequest:request
        completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (error || !data) {
                    m_status.isValid = false;
                    m_status.message = error ? [error.localizedDescription UTF8String] : "Server unreachable";
                    printf("[RemoteAttestation] Network error: %s\n", m_status.message.c_str());
                    if (callback) callback(m_status);
                    return;
                }

                NSError *parseErr = nil;
                NSDictionary *resJson = [NSJSONSerialization JSONObjectWithData:data options:0 error:&parseErr];
                if (parseErr || ![resJson isKindOfClass:[NSDictionary class]]) {
                    m_status.isValid = false;
                    m_status.message = "Invalid JSON response from server.";
                    printf("[RemoteAttestation] Protocol error: %s\n", m_status.message.c_str());
                    if (callback) callback(m_status);
                    return;
                }

                // Expected response format: { "valid": true, "message": "Active", "expires_at": 1780000000 }
                BOOL valid = [resJson[@"valid"] boolValue];
                NSString *msg = resJson[@"message"] ?: (valid ? @"Active" : @"Invalid Key");
                NSNumber *exp = resJson[@"expires_at"] ?: @(0);

                m_status.isValid = valid;
                m_status.key = key;
                m_status.message = [msg UTF8String];
                m_status.expiresAt = [exp longLongValue];

                if (valid) {
                    SaveLicenseKey(key);
                    printf("[RemoteAttestation] Valid license saved: %s\n", key.c_str());
                } else {
                    printf("[RemoteAttestation] Server rejected key: %s\n", m_status.message.c_str());
                }

                if (callback) callback(m_status);
            });
        }];

    [task resume];
}

} // namespace fluorite
