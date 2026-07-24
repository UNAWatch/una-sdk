/**
 ******************************************************************************
 * @file    VariantConfig.hpp
 * @brief   App-side reader for the embedded variant config.
 * @details A variant is a code-less alias .uapp in the app's own sandbox
 *          directory (see Docs/Multi-Activity-Apps-Design.md in the kernel
 *          repo). At startup the app opens the single .uapp in its sandbox
 *          root: a real binary means the app IS the base activity (all
 *          defaults apply); an alias carries an embedded JSON config that
 *          renames the activity and adjusts its FIT identity and
 *          family-specific features. A bad or unreadable config must never
 *          brick a launch, so every getter falls back to the caller's
 *          default.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef SDK_VARIANT_CONFIG_HPP
#define SDK_VARIANT_CONFIG_HPP

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/JSON/JsonStreamReader.hpp"

#include <cstdint>
#include <memory>

namespace SDK::Variant {

class Config {
public:
    /**
     * @brief   Discover the variant identity from the app's sandbox root.
     * @param   kernel: The app's kernel facade (service or GUI side).
     */
    explicit Config(const SDK::Kernel &kernel);

    /// True when a valid alias with a parseable config was found.
    bool isVariant() const { return mIsVariant; }

    /**
     * @brief   The variant's display name for in-app titles.
     * @param   defaultName: Returned for the base activity (no alias).
     */
    const char *name(const char *defaultName) const
    {
        return (mIsVariant && mName[0] != '\0') ? mName : defaultName;
    }

    /// FIT sport, defaulting to the family binary's classic value.
    uint8_t fitSport(uint8_t defaultSport) const
    {
        return mHasSport ? mSport : defaultSport;
    }

    /// FIT sub-sport, defaulting to the family binary's classic value.
    uint8_t fitSubSport(uint8_t defaultSubSport) const
    {
        return mHasSubSport ? mSubSport : defaultSubSport;
    }

    /**
     * @brief   Family-vocabulary lookups under the "features" subtree.
     *
     * The subtree is opaque to this shared parser: each family documents its
     * own keys and unknown entries are inert. Missing key or no variant ==>
     * the caller's default.
     */
    bool featureBool(const char *key, bool defaultValue) const;
    uint32_t featureU32(const char *key, uint32_t defaultValue) const;

private:
    /// Config schema major this parser understands. An unknown major falls
    /// back to defaults entirely (never a bricked launch).
    static constexpr uint32_t skSchemaSupported = 1;

    /// Max name length including the terminator (matches the launcher field).
    static constexpr size_t skNameLen = 16;

    bool mIsVariant = false;
    char mName[skNameLen] {};
    bool mHasSport = false;
    bool mHasSubSport = false;
    uint8_t mSport = 0;
    uint8_t mSubSport = 0;

    /// The embedded JSON, kept for feature queries (<= 8 KB by contract).
    std::unique_ptr<char[]> mConfig;
    size_t mConfigLen = 0;

    bool queryFeature(const char *key, SDK::JsonStreamReader &reader,
                      char *queryBuf, size_t queryBufSize) const;
};

} // namespace SDK::Variant

#endif // SDK_VARIANT_CONFIG_HPP
