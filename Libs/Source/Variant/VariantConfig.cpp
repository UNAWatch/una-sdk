/**
 ******************************************************************************
 * @file    VariantConfig.cpp
 * @brief   App-side reader for the embedded variant config.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Variant/VariantConfig.hpp"

#include <cstdio>
#include <cstring>

namespace SDK::Variant {

namespace {

// On-disk .uapp layout facts this reader depends on (the authoritative
// definition lives kernel-side in App/AppHeaders.hpp; the byte layout is
// locked by the kernel's host tests and by make_variant.py).
constexpr uint32_t kFlagVariantAlias = 0x40;
constexpr size_t   kMainHeaderSize   = 48;
constexpr size_t   kFlagsOffset      = 20;   // after uappID u64 + 3x u32
constexpr size_t   kPayloadOffset    = kMainHeaderSize + 3600 + 900;
constexpr size_t   kConfigOffset     = kPayloadOffset + 32;
constexpr size_t   kConfigSizeOffset = kPayloadOffset + 17;  // u32, unaligned
constexpr uint32_t kConfigSizeMax    = 8192;

bool hasUappExtension(const char *name)
{
    constexpr char kExt[] = ".uapp";
    size_t len = strlen(name);
    return len > strlen(kExt) && strcmp(&name[len - strlen(kExt)], kExt) == 0;
}

} // namespace

Config::Config(const SDK::Kernel &kernel)
{
    // The sandbox root holds exactly one .uapp: the app's own binary, or the
    // alias this variant was launched through.
    char path[SDK::Interface::IFileSystem::skMaxPathLen] {};
    {
        std::unique_ptr<SDK::Interface::IDirectory> dir = kernel.fs.dir("/");
        if (!dir || !dir->open()) {
            return;
        }

        SDK::Interface::IFileSystem::ObjectInfo item {};
        while (dir->readNext(item)) {
            if (!item.isDir && hasUappExtension(item.name)) {
                snprintf(path, sizeof(path), "/%s", item.name);
                break;
            }
        }
        dir->close();
    }

    if (path[0] == '\0') {
        return;
    }

    std::unique_ptr<SDK::Interface::IFile> file = kernel.fs.file(path);
    if (!file || !file->open()) {
        return;
    }

    bool ok = false;
    uint32_t flags = 0;
    uint32_t configSize = 0;

    do {
        size_t br = 0;
        if (!file->seek(kFlagsOffset) ||
                !file->read(reinterpret_cast<char*>(&flags), sizeof(flags), br)) {
            break;
        }

        if ((flags & kFlagVariantAlias) == 0) {
            // A real binary: the app is its classic self.
            break;
        }

        if (!file->seek(kConfigSizeOffset) ||
                !file->read(reinterpret_cast<char*>(&configSize), sizeof(configSize), br)) {
            break;
        }

        if (configSize == 0 || configSize > kConfigSizeMax) {
            break;
        }

        mConfig = std::unique_ptr<char[]>(new (std::nothrow) char[configSize]);
        if (!mConfig) {
            break;
        }

        if (!file->seek(kConfigOffset) ||
                !file->read(mConfig.get(), configSize, br) || br != configSize) {
            mConfig.reset();
            break;
        }

        ok = true;
    } while (false);

    file->close();

    if (!ok) {
        return;
    }

    SDK::JsonStreamReader reader(mConfig.get(), configSize);
    if (!reader.validate()) {
        mConfig.reset();
        return;
    }

    uint32_t schema = 0;
    if (!reader.get("schema", schema) || schema != skSchemaSupported) {
        // Unknown major: run as the classic self rather than misread keys.
        mConfig.reset();
        return;
    }

    mConfigLen = configSize;
    mIsVariant = true;

    const char *name = nullptr;
    size_t nameLen = 0;
    if (reader.get("name", name, nameLen)) {
        snprintf(mName, sizeof(mName), "%.*s", static_cast<int>(nameLen), name);
    }

    mHasSport = reader.get("fit.sport", mSport);
    mHasSubSport = reader.get("fit.subSport", mSubSport);

}

bool Config::featureBool(const char *key, bool defaultValue) const
{
    if (!mIsVariant || !mConfig) {
        return defaultValue;
    }

    SDK::JsonStreamReader reader(mConfig.get(), mConfigLen);
    char query[64];
    bool value = defaultValue;
    if (!queryFeature(key, reader, query, sizeof(query)) ||
            !reader.get(query, value)) {
        return defaultValue;
    }
    return value;
}

uint32_t Config::featureU32(const char *key, uint32_t defaultValue) const
{
    if (!mIsVariant || !mConfig) {
        return defaultValue;
    }

    SDK::JsonStreamReader reader(mConfig.get(), mConfigLen);
    char query[64];
    uint32_t value = defaultValue;
    if (!queryFeature(key, reader, query, sizeof(query)) ||
            !reader.get(query, value)) {
        return defaultValue;
    }
    return value;
}

bool Config::queryFeature(const char *key, SDK::JsonStreamReader &reader,
                          char *queryBuf, size_t queryBufSize) const
{
    (void)reader;
    int written = snprintf(queryBuf, queryBufSize, "features.%s", key);
    return written > 0 && static_cast<size_t>(written) < queryBufSize;
}

} // namespace SDK::Variant
