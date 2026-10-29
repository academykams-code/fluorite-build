//
//  payload_loader.h
//  Reconstructed from libloader (Embedded Archive & Payload Unpacker)
//
//  Locates, decompresses, and extracts the fluorite_resources.tar.lz4 payload
//

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace loader {

enum class PayloadVariant {
    Primary = 0, // "fs/x_data.bin" (83,715,776 bytes Mach-O MH_EXECUTE)
    Secondary = 1 // "fs/y_data.bin" (83,715,776 bytes Mach-O MH_EXECUTE)
};

struct ArchiveEntryInfo {
    std::string path;
    uint64_t size{0};
    bool isDirectory{false};
};

class PayloadLoader {
public:
    static PayloadLoader& Instance();

    /**
     * Locates "fluorite_resources.tar.lz4" in the main application bundle.
     * @return Absolute path to fluorite_resources.tar.lz4
     */
    std::string GetResourceArchivePath() const;

    /**
     * Unpacks and extracts fluorite_resources.tar.lz4 using the embedded
     * libarchive / LZ4 decompression pipeline into the target directory.
     * 
     * @param destinationDirectory Directory to extract payloads and resources
     * @return true if extraction succeeded
     */
    bool ExtractPayload(const std::string& destinationDirectory);

    /**
     * In-memory extraction of the main game executable payload directly
     * without writing unencrypted bytes to disk.
     * 
     * @param outPayloadBuffer Destination buffer receiving the decrypted Mach-O binary
     * @param variant Selects either Primary (fs/x_data.bin) or Secondary (fs/y_data.bin)
     * @return true if payload was found and successfully decompressed
     */
    bool ExtractPayloadToMemory(std::vector<uint8_t>& outPayloadBuffer,
                                PayloadVariant variant = PayloadVariant::Primary);

    /**
     * Lists all file entries stored inside fluorite_resources.tar.lz4.
     */
    std::vector<ArchiveEntryInfo> ListArchiveEntries();

private:
    PayloadLoader() = default;
    std::string m_archiveFilename{"fluorite_resources.tar.lz4"};
    std::string m_primaryPayloadName{"fs/x_data.bin"};
    std::string m_secondaryPayloadName{"fs/y_data.bin"};
};

} // namespace loader
