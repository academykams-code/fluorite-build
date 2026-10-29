//
//  remote_attestation.h
//  Fluorite Reconstructed
//
//  HTTP REST License Server Verification Client & Key Store
//

#pragma once

#include <string>
#include <memory>
#include <functional>

namespace fluorite {

struct LicenseStatus {
    bool isValid{false};
    std::string key;
    std::string message;
    int64_t expiresAt{0};
};

class RemoteAttestation {
public:
    static RemoteAttestation& Instance();

    // Initialize & verify stored license key
    bool Initialize(const std::string& fallbackKey = "");

    // Key management (persisted in iOS NSUserDefaults)
    std::string GetSavedLicenseKey() const;
    void SaveLicenseKey(const std::string& key);

    // License Server Configuration
    void SetServerUrl(const std::string& url) { m_serverUrl = url; }
    const std::string& GetServerUrl() const { return m_serverUrl; }

    // Verification routines
    void VerifyLicense(const std::string& key,
                       std::function<void(const LicenseStatus& status)> callback = nullptr);
    void VerifySavedLicense(std::function<void(const LicenseStatus& status)> callback = nullptr);

    bool IsLicenseValid() const { return m_status.isValid; }
    const LicenseStatus& Status() const { return m_status; }

private:
    RemoteAttestation();
    std::string m_serverUrl{"https://insect-proven-mentally.ngrok-free.app/api/verify_license"};
    LicenseStatus m_status;
};

} // namespace fluorite
