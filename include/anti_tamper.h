//
//  anti_tamper.h
//  Reconstructed from libloader (Integrity Seal & Anti-Analysis Engine)
//
//  Mach-O Code Integrity Verification, Frida / Substrate Detection, and Ad Suppression
//

#pragma once

#include <cstdint>
#include <string>

namespace security {

#pragma pack(push, 1)
struct FluoIntegritySeal {
    char magic[16];          // "FLUOINTEGRITY001"
    uint32_t version;        // 2
    uint32_t flags;          // 2
    uint64_t sealAddress;    // 0xe84040
    uint64_t textStart;      // 0x8000
    uint64_t textSize;       // 0xba7f74
    uint8_t expectedSha256[32];
    uint8_t ecdsaSignature[64];
};
#pragma pack(pop)

class AntiTamper {
public:
    static AntiTamper& Instance();

    bool VerifyBinaryIntegrity();
    bool DetectAnalysisTools();
    void InstallAdBlockHooks();

private:
    AntiTamper() = default;

    bool CheckFridaLoaded();
    bool CheckSubstrateLoaded();
    bool CheckDyldEnvironment();
};

} // namespace security
