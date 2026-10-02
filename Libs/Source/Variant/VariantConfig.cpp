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
constexpr uint32_t kFlagVariantAlias  = 0x40;
constexpr size_t   kMainHeaderSize    = 48;
constexpr size_t   kFlagsOffset       = 20;   // after uappID u64 + 3x u32
constexpr size_t   kPayloadOffset     = kMainHeaderSize + 3600 + 900;
constexpr size_t   kConfigOffset      = kPayloadOffset + 32;
constexpr size_t   kConfigSizeOffset  = kPayloadOffset + 17;  // u32, unaligned
constexpr uint32_t kConfigSizeMax     = 8192;
constexpr uint32_t kPayloadVersion    = 1;    // the layout this reader parses

bool hasUappExtension(const char *name)
{
    constexpr char kExt[] = ".uapp";
    size_t len = strlen(name);
    return len > strlen(kExt) && strcmp(&name[len - strlen(kExt)], kExt) == 0;
}

/// Read the app-flags word of a .uapp; false when unreadable.
bool readFlags(const SDK::Kernel &kernel, const char *path, uint32_t &flags)
{
    std::unique_ptr<SDK::Interface::IFile> file = kernel.fs.file(path);
    if (!file || !file->open()) {
        return false;
    }
    size_t br = 0;
    bool ok = file->seek(kFlagsOffset) &&
            file->read(reinterpret_cast<char*>(&flags), sizeof(flags), br) &&
            br == sizeof(flags);
    file->close();
    return ok;
}

} // namespace

Config::Config(const SDK::Kernel &kernel)
{
    // Pick the same file the kernel's scan picked (the mixed-directory
    // determinism rules): any real (non-alias) .uapp in the sandbox
    // root means the app runs as its classic self; otherwise the first
    // alias-flagged .uapp is this variant's identity. An unreadable
    // candidate counts as a real app, mirroring the kernel's routing.
    char path[SDK::Interface::IFileSystem::skMaxPathLen] {};
    {
        std::unique_ptr<SDK::Interface::IDirectory> dir = kernel.fs.dir("/");
        if (!dir || !dir->open()) {
            return;
        }

        SDK::Interface::IFileSystem::ObjectInfo item {};
        bool realAppSeen = false;
        while (dir->readNext(item)) {
            if (item.isDir || !hasUappExtension(item.name)) {
                continue;
            }
            // Per iteration and zero-initialised, so an entry whose path is
            // declined below names nothing at all rather than inheriting what
            // the last one left here.
            char candidate[SDK::Interface::IFileSystem::skMaxPathLen] {};

            // ObjectInfo::name is as wide as a whole path, so "/" + name need
            // not fit in one. A name too long to form a path is as unreadable
            // as one whose flags will not load, and takes the same
            // conservative branch: a truncated path would name some other
            // file, and opening the wrong .uapp is worse than opening none.
            //
            // Measuring first, rather than recovering the length from
            // snprintf, is what keeps the path free of anything to truncate --
            // and with it the -Wformat-truncation that a "/%s" whose source is
            // as wide as its destination earns at the -Os -Wall app builds use.
            const size_t nameLen = strlen(item.name);
            // The path is '/' + name + '\0', so nameLen + 2 bytes.
            const bool nameFits = nameLen + 2 <= sizeof(candidate);
            if (nameFits) {
                candidate[0] = '/';
                memcpy(&candidate[1], item.name, nameLen + 1); // includes the NUL
            }

            uint32_t flags = 0;
            if (!nameFits || !readFlags(kernel, candidate, flags) ||
                    (flags & kFlagVariantAlias) == 0) {
                realAppSeen = true;
                break;
            }
            if (path[0] == '\0') {
                snprintf(path, sizeof(path), "%s", candidate);
            }
        }
        dir->close();

        if (realAppSeen) {
            path[0] = '\0';
        }
    }

    if (path[0] == '\0') {
        // The app is its classic self.
        return;
    }

    std::unique_ptr<SDK::Interface::IFile> file = kernel.fs.file(path);
    if (!file || !file->open()) {
        return;
    }

    bool ok = false;
    uint32_t payloadVersion = 0;
    uint32_t configSize = 0;

    do {
        size_t br = 0;

        // Only the payload layout this reader was built for: a future kernel
        // may accept newer payloads, but a shipped binary must never guess
        // at rearranged fields -- unknown version ==> classic defaults.
        if (!file->seek(kPayloadOffset) ||
                !file->read(reinterpret_cast<char*>(&payloadVersion),
                        sizeof(payloadVersion), br) ||
                br != sizeof(payloadVersion) ||
                payloadVersion != kPayloadVersion) {
            break;
        }

        if (!file->seek(kConfigSizeOffset) ||
                !file->read(reinterpret_cast<char*>(&configSize), sizeof(configSize), br) ||
                br != sizeof(configSize)) {
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
        // Clipping is the right answer here, unlike for the candidate path
        // above: mName is a display label, so a too-long one shows shortened
        // rather than failing the launch. snprintf always NUL-terminates.
        // -Wformat-truncation=2 flags this, which is the intended behaviour
        // and not a level app builds enable.
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
