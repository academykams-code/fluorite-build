//
//  payload_loader.cpp
//  Reconstructed from libloader (Embedded Archive & Payload Unpacker)
//
//  Decompression pipeline for fluorite_resources.tar.lz4 via libarchive & LZ4
//

#include "payload_loader.h"
#import <Foundation/Foundation.h>
#include <cstdio>
#include <cstring>

// Embedded libarchive C declarations matching libloader symbol table
extern "C" {
    struct archive {};
    struct archive_entry {};

    __attribute__((weak)) struct archive *archive_read_new(void) { return nullptr; }
    __attribute__((weak)) int archive_read_support_filter_lz4(struct archive *) { return 0; }
    __attribute__((weak)) int archive_read_support_format_tar(struct archive *) { return 0; }
    __attribute__((weak)) int archive_read_open_filename(struct archive *, const char *, size_t) { return -1; }
    __attribute__((weak)) int archive_read_next_header(struct archive *, struct archive_entry **) { return -1; }
    __attribute__((weak)) const char *archive_entry_pathname(struct archive_entry *) { return ""; }
    __attribute__((weak)) int64_t archive_entry_size(struct archive_entry *) { return 0; }
    __attribute__((weak)) int archive_entry_filetype(struct archive_entry *) { return 0; }
    __attribute__((weak)) int64_t archive_read_data(struct archive *, void *, size_t) { return 0; }
    __attribute__((weak)) int archive_read_close(struct archive *) { return 0; }
    __attribute__((weak)) int archive_read_free(struct archive *) { return 0; }
    __attribute__((weak)) const char *archive_error_string(struct archive *) { return "archive not available"; }
}

namespace loader {

PayloadLoader& PayloadLoader::Instance() {
    static PayloadLoader s_instance;
    return s_instance;
}

std::string PayloadLoader::GetResourceArchivePath() const {
    // 1. Look in main application bundle: pool.app/fluorite_resources.tar.lz4
    NSBundle *mainBundle = [NSBundle mainBundle];
    NSString *bundlePath = [mainBundle bundlePath];
    NSString *archivePath = [bundlePath stringByAppendingPathComponent:[NSString stringWithUTF8String:m_archiveFilename.c_str()]];

    if ([[NSFileManager defaultManager] fileExistsAtPath:archivePath]) {
        return std::string([archivePath UTF8String]);
    }

    // 2. Fallback to resource path lookup
    NSString *resPath = [mainBundle pathForResource:@"fluorite_resources" ofType:@"tar.lz4"];
    if (resPath && [[NSFileManager defaultManager] fileExistsAtPath:resPath]) {
        return std::string([resPath UTF8String]);
    }

    return "";
}

std::vector<ArchiveEntryInfo> PayloadLoader::ListArchiveEntries() {
    std::vector<ArchiveEntryInfo> entries;
    std::string archivePath = GetResourceArchivePath();
    if (archivePath.empty()) {
        printf("[PayloadLoader] Archive %s not found in application bundle.\n", m_archiveFilename.c_str());
        return entries;
    }

    struct archive *a = archive_read_new();
    archive_read_support_filter_lz4(a);
    archive_read_support_format_tar(a);

    if (archive_read_open_filename(a, archivePath.c_str(), 65536) != 0) {
        printf("[PayloadLoader] Failed to open archive: %s\n", archive_error_string(a));
        archive_read_free(a);
        return entries;
    }

    struct archive_entry *entry = nullptr;
    while (archive_read_next_header(a, &entry) == 0) {
        ArchiveEntryInfo info;
        info.path = archive_entry_pathname(entry);
        info.size = archive_entry_size(entry);
        entries.push_back(info);
    }

    archive_read_close(a);
    archive_read_free(a);
    return entries;
}

bool PayloadLoader::ExtractPayload(const std::string& destinationDirectory) {
    std::string archivePath = GetResourceArchivePath();
    if (archivePath.empty()) {
        printf("[PayloadLoader] Missing %s\n", m_archiveFilename.c_str());
        return false;
    }

    printf("[PayloadLoader] Decompressing %s -> %s...\n", archivePath.c_str(), destinationDirectory.c_str());

    struct archive *a = archive_read_new();
    archive_read_support_filter_lz4(a);
    archive_read_support_format_tar(a);

    if (archive_read_open_filename(a, archivePath.c_str(), 65536) != 0) {
        printf("[PayloadLoader] archive_read_open_filename failed: %s\n", archive_error_string(a));
        archive_read_free(a);
        return false;
    }

    NSFileManager *fm = [NSFileManager defaultManager];
    NSString *destDir = [NSString stringWithUTF8String:destinationDirectory.c_str()];
    [fm createDirectoryAtPath:destDir withIntermediateDirectories:YES attributes:nil error:nil];

    struct archive_entry *entry = nullptr;
    bool foundPayload = false;

    while (archive_read_next_header(a, &entry) == 0) {
        const char *entryPath = archive_entry_pathname(entry);
        int64_t entrySize = archive_entry_size(entry);

        NSString *outPath = [destDir stringByAppendingPathComponent:[NSString stringWithUTF8String:entryPath]];

        // Ensure parent directory exists
        NSString *parentDir = [outPath stringByDeletingLastPathComponent];
        [fm createDirectoryAtPath:parentDir withIntermediateDirectories:YES attributes:nil error:nil];

        FILE *outFile = fopen([outPath UTF8String], "wb");
        if (outFile) {
            std::vector<char> buffer(65536);
            int64_t bytesRead = 0;
            while ((bytesRead = archive_read_data(a, buffer.data(), buffer.size())) > 0) {
                fwrite(buffer.data(), 1, bytesRead, outFile);
            }
            fclose(outFile);

            if (strcmp(entryPath, m_primaryPayloadName.c_str()) == 0 ||
                strcmp(entryPath, m_secondaryPayloadName.c_str()) == 0) {
                foundPayload = true;
                printf("[PayloadLoader] Successfully extracted Mach-O payload: %s (%lld bytes)\n",
                       entryPath, entrySize);
            }
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    return foundPayload;
}

bool PayloadLoader::ExtractPayloadToMemory(std::vector<uint8_t>& outPayloadBuffer, PayloadVariant variant) {
    std::string archivePath = GetResourceArchivePath();
    if (archivePath.empty()) {
        return false;
    }

    const std::string& targetName = (variant == PayloadVariant::Primary) ? m_primaryPayloadName : m_secondaryPayloadName;

    struct archive *a = archive_read_new();
    archive_read_support_filter_lz4(a);
    archive_read_support_format_tar(a);

    if (archive_read_open_filename(a, archivePath.c_str(), 65536) != 0) {
        archive_read_free(a);
        return false;
    }

    struct archive_entry *entry = nullptr;
    bool success = false;

    while (archive_read_next_header(a, &entry) == 0) {
        const char *entryPath = archive_entry_pathname(entry);
        if (strcmp(entryPath, targetName.c_str()) == 0) {
            int64_t size = archive_entry_size(entry);
            outPayloadBuffer.resize(size);

            int64_t totalRead = 0;
            while (totalRead < size) {
                int64_t bytes = archive_read_data(a, outPayloadBuffer.data() + totalRead, size - totalRead);
                if (bytes <= 0) break;
                totalRead += bytes;
            }

            printf("[PayloadLoader] Decrypted and loaded %s into memory (%lld bytes, Mach-O magic: 0x%08X)\n",
                   entryPath, size, *(uint32_t*)outPayloadBuffer.data());
            success = (totalRead == size);
            break;
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    return success;
}

} // namespace loader
