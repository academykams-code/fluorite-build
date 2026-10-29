//
//  import_manager.h
//  Reconstructed from libloader (C++ RTTI & Account Manager Metadata)
//
//  Guest Token & Account Management for 8 Ball Pool
//

#pragma once

#include <string>
#include <vector>

namespace fluorite {

struct GuestAccount {
    std::string token;
    std::string domain{"guestTokenDomain"};
    std::string plistPath;
    std::string alias;
    int64_t createdAt{0};
};

class ImportManager {
public:
    static ImportManager& Instance();

    bool ExportCurrentGuestToken(const std::string& destinationDirectory);
    bool ImportGuestTokenFromFile(const std::string& sourcePlistPath);
    std::string GetCurrentGuestToken();

    void PresentDocumentPickerForImport();
    void PresentDocumentPickerForExport();

private:
    ImportManager() = default;
};

} // namespace fluorite
