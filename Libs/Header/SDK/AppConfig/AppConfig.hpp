/**
 ******************************************************************************
 * @file    AppConfig.hpp
 * @brief   App-side reader and writer for developer-declared config fields.
 * @details An app declares configuration fields in its config.json; the
 *          companion app collects values from the user and writes them to a
 *          JSON file in the app's own directory on the watch. This class reads
 *          that file, and can write it back when the user changes a value on
 *          the watch. The contract is documented in
 *          Docs/app-config-fields.md.
 *
 *          config.json never reaches the watch, so the app carries its own copy
 *          of the field contract: one constexpr Field table giving each id its
 *          type, default and bounds. CI compares that table against config.json
 *          (validate_app_config.py --check-bounds), which is what makes it safe
 *          to clamp a value the app should never have received.
 *
 *          Nothing here may stop an app from starting. A missing, oversized,
 *          malformed or future-schema file yields the declared defaults; a
 *          single bad key falls back to that field's default alone.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef SDK_APP_CONFIG_HPP
#define SDK_APP_CONFIG_HPP

#include "SDK/Kernel/Kernel.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace SDK {

/**
 * @brief   Reads (and optionally writes) an app's configuration values.
 *
 * Typical use, once, at startup:
 * @code
 * constexpr SDK::AppConfig::Field kFields[] = {
 *     SDK::AppConfig::stringField("waypointName", "Waypoint", 1, 16),
 *     SDK::AppConfig::floatField ("targetLatitude", 51.5072f, -90.0f, 90.0f),
 *     SDK::AppConfig::intField   ("arrivalRadiusM", 25, 5, 500),
 *     SDK::AppConfig::boolField  ("vibrateOnArrival", true),
 * };
 *
 * SDK::AppConfig cfg(kernel, "app_config.json", kFields);
 * cfg.getString("waypointName", mName, sizeof(mName));
 * mRadiusM = cfg.getInt("arrivalRadiusM");
 * @endcode
 */
class AppConfig {
public:
    /// Field limits, matching Docs/app-config-fields.md section 8.
    static constexpr size_t   skMaxFields       = 32;
    static constexpr size_t   skMaxFileBytes    = 8192;
    static constexpr size_t   skMaxStringBytes  = 128;
    /// Longest configFile name accepted, including the terminator.
    static constexpr size_t   skMaxFileNameLen  = 64;
    /// Envelope schema this build understands; anything else means defaults.
    static constexpr uint32_t skSchemaSupported = 1;

    enum class Type : uint8_t {
        Bool,
        Int,
        Float,
        String,
    };

    /**
     * @brief   One declared field: its id, type, default and bounds.
     *
     * Build entries with the factory functions below rather than by hand, so
     * the table stays in the single-line form CI can parse.
     */
    struct Field {
        const char *id            = nullptr;
        Type        type          = Type::Bool;
        bool        boolDefault   = false;
        int32_t     intDefault    = 0;
        float       floatDefault  = 0.0f;
        const char *stringDefault = nullptr;
        int32_t     intMin        = 0;
        int32_t     intMax        = 0;
        float       floatMin      = 0.0f;
        float       floatMax      = 0.0f;
        uint16_t    minLength     = 0;
        uint16_t    maxLength     = 0;
    };

    /// Declare a `bool` field.
    static constexpr Field boolField(const char *id, bool defaultValue)
    {
        Field f {};
        f.id = id;
        f.type = Type::Bool;
        f.boolDefault = defaultValue;
        return f;
    }

    /// Declare an `int` field. Values are clamped to [min, max] on read.
    static constexpr Field intField(const char *id, int32_t defaultValue,
                                    int32_t min, int32_t max)
    {
        Field f {};
        f.id = id;
        f.type = Type::Int;
        f.intDefault = defaultValue;
        f.intMin = min;
        f.intMax = max;
        return f;
    }

    /// Declare a `float` field. Values are clamped to [min, max] on read.
    static constexpr Field floatField(const char *id, float defaultValue,
                                      float min, float max)
    {
        Field f {};
        f.id = id;
        f.type = Type::Float;
        f.floatDefault = defaultValue;
        f.floatMin = min;
        f.floatMax = max;
        return f;
    }

    /// Declare a `string` field. Lengths are in UTF-8 bytes.
    static constexpr Field stringField(const char *id, const char *defaultValue,
                                       uint16_t minLength, uint16_t maxLength)
    {
        Field f {};
        f.id = id;
        f.type = Type::String;
        f.stringDefault = defaultValue;
        f.minLength = minLength;
        f.maxLength = maxLength;
        return f;
    }

    /**
     * @brief   Open and read the app's configuration file.
     * @param   kernel: The app's kernel facade (service or GUI side).
     * @param   fileName: The bare filename declared as "configFile" in
     *          config.json. Resolved in the app's sandbox root.
     * @param   fields: The app's field table.
     * @param   fieldCount: Number of entries in @p fields, at most
     *          @ref skMaxFields.
     */
    AppConfig(const Kernel &kernel, const char *fileName,
              const Field *fields, size_t fieldCount);

    /// Convenience overload that takes the table's length from its type.
    template <size_t N>
    AppConfig(const Kernel &kernel, const char *fileName, const Field (&fields)[N])
        : AppConfig(kernel, fileName, fields, N)
    {
        static_assert(N <= skMaxFields, "an app may declare at most 32 config fields");
    }

    AppConfig(const AppConfig &) = delete;
    AppConfig &operator=(const AppConfig &) = delete;

    /// True when a values file was found, parsed, and had a supported schema.
    bool isLoaded() const { return mLoaded; }

    /**
     * @brief   Whether a usable value for @p id came from the file.
     *
     * Distinguishes "the user chose this" from "this is the app's default".
     * A key that was present but unusable (wrong JSON type, malformed number,
     * `null`) counts as absent.
     */
    bool has(const char *id) const;

    /// The stored value, or the declared default.
    bool getBool(const char *id) const;

    /// The stored value clamped to the declared range, or the default.
    int32_t getInt(const char *id) const;

    /// The stored value clamped to the declared range, or the default.
    /// A non-finite stored value is treated as malformed.
    float getFloat(const char *id) const;

    /**
     * @brief   Copy a string value out, NUL-terminated.
     * @param   id: Field id.
     * @param   out: Destination buffer.
     * @param   outSize: Size of @p out including the terminator.
     * @return  Bytes written, excluding the terminator.
     *
     * Truncated on a UTF-8 character boundary to fit both the field's declared
     * `maxLength` and @p outSize, so a partial multi-byte character is never
     * produced. JSON escapes in the stored value are decoded.
     */
    size_t getString(const char *id, char *out, size_t outSize) const;

    /// Set a value, clamping it into the declared range. False for a bad id.
    bool setBool(const char *id, bool value);
    bool setInt(const char *id, int32_t value);
    bool setFloat(const char *id, float value);

    /// Set a string value, truncating on a character boundary to `maxLength`.
    bool setString(const char *id, const char *value);

    /**
     * @brief   Forget a value, so the field falls back to its default.
     *
     * The key is removed from the file by the next @ref save.
     */
    bool clear(const char *id);

    /// True when there are unsaved changes.
    bool isDirty() const { return mDirty; }

    /**
     * @brief   Write the values file back, if anything changed.
     * @return  True if the file is up to date (including "nothing to do").
     *
     * Writes to a temporary file and renames it over the original, so an
     * interrupted save cannot destroy the previous values. Keys the app never
     * declared are copied through unchanged; declared keys are written only
     * when they were set by the user or by this app, so a value left at its
     * default does not become an explicit entry.
     */
    bool save();

private:
    /// Per-field runtime state. Only populated for fields the app writes.
    struct Slot {
        bool    boolValue  = false;
        int32_t intValue   = 0;
        float   floatValue = 0.0f;
        std::unique_ptr<char[]> stringValue;
    };

    /// A value's location in the raw document: [start, start + length).
    struct Slice {
        const char *data = nullptr;
        size_t      length = 0;
    };

    const Field *find(const char *id, size_t &indexOut) const;
    bool readFile();
    /// Complete an interrupted save (temporary file present, real file gone).
    void recoverInterruptedSave();
    void indexPresentValues();

    /// Locate a field's value in the document, checking its JSON type.
    bool rawSlice(const Field &field, Slice &out) const;

    /// Read a stored value. False when absent or unusable, leaving @p out alone.
    bool fileBool(const Field &field, bool &out) const;
    bool fileInt(const Field &field, int32_t &out) const;
    bool fileFloat(const Field &field, float &out) const;
    bool fileString(const Field &field, char *out, size_t outSize,
                    size_t &written) const;

    bool writeDocument(Interface::IFile &file) const;

    /// The write slot for a field, allocating the table on first use.
    /// Null when that allocation failed, so no caller can index nothing.
    Slot *slot(size_t index);

    const Kernel &mKernel;
    const Field  *mFields;
    size_t        mCount;

    char mPath[skMaxFileNameLen + 2] {};       ///< "/<fileName>"
    char mTmpPath[skMaxFileNameLen + 6] {};    ///< "/<fileName>.tmp"

    std::unique_ptr<char[]> mRaw;              ///< The file, as read.
    size_t mRawLen = 0;
    Slice  mValues {};                          ///< The "values" object text.

    std::unique_ptr<Slot[]> mSlots;             ///< Allocated on first write.

    uint32_t mPresent  = 0;   ///< Bit set: a usable value came from the file.
    uint32_t mOverride = 0;   ///< Bit set: the app supplied a value.
    uint32_t mRemoved  = 0;   ///< Bit set: cleared by the app.

    bool mLoaded = false;
    bool mDirty  = false;
};

} // namespace SDK

#endif // SDK_APP_CONFIG_HPP
