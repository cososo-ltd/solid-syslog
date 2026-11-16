#ifndef LITTLEFSSTORESTACK_H
#define LITTLEFSSTORESTACK_H

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "LittleFsDisk.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogLittleFsFile.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStoreDefinition.h"
#include "lfs.h"
}

/* The whole stack over one emulated flash part. Mount and Unmount bracket the
 * library state, so a test can throw all of it away and come back to the same
 * bytes - which is what a reboot does. */
struct LittleFsStoreStack
{
    LittleFsDisk disk = {};
    lfs_config config = {};
    lfs_t filesystem = {};
    unsigned char fileCache[LITTLEFS_DISK_CACHE_SIZE] = {};
    SolidSyslogFile* file = nullptr;
    SolidSyslogBlockDevice* device = nullptr;
    SolidSyslogStore* store = nullptr;
    /* The size of one store block, which is one file, and how many the store
     * may keep. Set before FormatAndMount. */
    size_t storeBlockBytes = 1024;
    size_t storeMaxBlocks = 8;

    void FormatAndMount()
    {
        LittleFsDisk_Init(&disk);
        LittleFsDisk_Configure(&disk, &config);
        LONGS_EQUAL(LFS_ERR_OK, lfs_format(&filesystem, &config));
        Mount();
    }

    void Mount()
    {
        LONGS_EQUAL(LFS_ERR_OK, lfs_mount(&filesystem, &config));
        file = SolidSyslogLittleFsFile_Create(&filesystem, fileCache, sizeof(fileCache));
        device = SolidSyslogFileBlockDevice_Create(file, "/STORE", storeBlockBytes);
        SolidSyslogBlockStoreConfig storeConfig = {};
        storeConfig.BlockDevice = device;
        storeConfig.MaxBlocks = storeMaxBlocks;
        store = SolidSyslogBlockStore_Create(&storeConfig);
    }

    /* Release every handle the library holds, as losing power would. The
     * storage is untouched. */
    void Unmount()
    {
        /* Nothing to release where a mount never finished. */
        if (store != nullptr)
        {
            SolidSyslogBlockStore_Destroy(store);
            SolidSyslogFileBlockDevice_Destroy(device);
            SolidSyslogLittleFsFile_Destroy(file);
            store = nullptr;
            device = nullptr;
            file = nullptr;
            (void) lfs_unmount(&filesystem);
        }
    }

    /* A reboot: the library state and the lfs_t go, the bytes stay. The
     * unmount and fresh mount are what actually discard LittleFS's in-RAM
     * state; zeroing the struct as well is belt-and-braces that no test
     * distinguishes, kept because it says what a reboot is. */
    void Reboot()
    {
        Unmount();
        std::memset(&filesystem, 0, sizeof(filesystem));
        LittleFsDisk_PowerOn(&disk);
        Mount();
    }

    [[nodiscard]] bool Write(const std::string& record) const
    {
        return SolidSyslogStore_Write(store, record.data(), record.size()) == SOLIDSYSLOG_STORE_WRITE_KEPT;
    }

    [[nodiscard]] std::vector<std::string> DrainUnsent() const
    {
        std::vector<std::string> records;
        char buffer[256] = {};
        size_t bytesRead = 0U;
        while (SolidSyslogStore_ReadNextUnsent(store, buffer, sizeof(buffer), &bytesRead) && (bytesRead > 0))
        {
            records.emplace_back(buffer, bytesRead);
            SolidSyslogStore_MarkSent(store);
            bytesRead = 0;
        }
        return records;
    }
};

#endif /* LITTLEFSSTORESTACK_H */
