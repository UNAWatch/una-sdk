/**
 ******************************************************************************
 * @file    FitReader.hpp
 * @brief   Minimal independent FIT decoder for host tests.
 *
 * Parses the header, walks definition/data records, decodes each data message's
 * fields by the active local definition, and verifies the trailing file CRC.
 * Test-only; not part of the SDK.
 ******************************************************************************
 */

#ifndef __TEST_FIT_READER_HPP
#define __TEST_FIT_READER_HPP

#include "SDK/Fit/FitCrc.hpp"

#include <cstdint>
#include <map>
#include <vector>

namespace testfit {

class FitReader {
public:
    struct FieldVal {
        uint8_t              baseType = 0;
        std::vector<uint8_t> raw;
        uint64_t u() const
        {
            uint64_t v = 0;
            for (size_t i = 0; i < raw.size(); ++i) v |= uint64_t(raw[i]) << (8 * i);
            return v;
        }
    };
    struct Message {
        uint16_t                    global = 0;
        std::map<uint8_t, FieldVal> fields;
        std::map<uint8_t, FieldVal> devFields;
    };

    explicit FitReader(const std::vector<uint8_t>& b) : mData(b) { parse(); }

    bool ok() const { return mOk; }
    bool crcValid() const { return mCrcValid; }
    uint8_t headerSize() const { return mHeaderSize; }
    const std::vector<Message>& messages() const { return mMsgs; }

    std::vector<const Message*> withGlobal(uint16_t g) const
    {
        std::vector<const Message*> out;
        for (const auto& m : mMsgs) if (m.global == g) out.push_back(&m);
        return out;
    }

private:
    struct FieldDef { uint8_t num, size, baseType; };
    struct Def { uint16_t global; std::vector<FieldDef> fields, devFields; };

    void parse()
    {
        if (mData.size() < 14u + 2u) return;
        mHeaderSize = mData[0];
        if (mHeaderSize != 14) return;
        if (mData[8] != '.' || mData[9] != 'F' || mData[10] != 'I' || mData[11] != 'T') return;

        const uint32_t dataSize = mData[4] | (mData[5] << 8) | (mData[6] << 16) | (uint32_t(mData[7]) << 24);
        const size_t   dataEnd  = mHeaderSize + dataSize;
        if (dataEnd + 2 != mData.size()) return;

        const uint16_t want = mData[dataEnd] | (mData[dataEnd + 1] << 8);
        mCrcValid = (SDK::Fit::fitCrcUpdate(0, mData.data(), dataEnd) == want);

        std::map<uint8_t, Def> defs;
        size_t p = mHeaderSize;
        while (p < dataEnd) {
            const uint8_t hdr = mData[p++];
            if (hdr & 0x80) return;
            const uint8_t local = hdr & 0x0F;
            if (hdr & 0x40) {
                Def d;
                p++;  // reserved
                p++;  // architecture
                d.global = mData[p] | (mData[p + 1] << 8); p += 2;
                const uint8_t nf = mData[p++];
                for (uint8_t i = 0; i < nf; ++i) { d.fields.push_back({mData[p], mData[p+1], mData[p+2]}); p += 3; }
                if (hdr & 0x20) {
                    const uint8_t nd = mData[p++];
                    for (uint8_t i = 0; i < nd; ++i) { d.devFields.push_back({mData[p], mData[p+1], mData[p+2]}); p += 3; }
                }
                defs[local] = d;
            } else {
                auto it = defs.find(local);
                if (it == defs.end()) return;
                Message m;
                m.global = it->second.global;
                for (const auto& f : it->second.fields) {
                    FieldVal v; v.baseType = f.baseType;
                    v.raw.assign(mData.begin() + p, mData.begin() + p + f.size); p += f.size;
                    m.fields[f.num] = v;
                }
                for (const auto& f : it->second.devFields) {
                    FieldVal v; v.baseType = 0;
                    v.raw.assign(mData.begin() + p, mData.begin() + p + f.size); p += f.size;
                    m.devFields[f.num] = v;
                }
                mMsgs.push_back(std::move(m));
            }
        }
        mOk = (p == dataEnd);
    }

    const std::vector<uint8_t>& mData;
    std::vector<Message>        mMsgs;
    bool                        mOk        = false;
    bool                        mCrcValid  = false;
    uint8_t                     mHeaderSize = 0;
};

}  // namespace testfit

#endif  // __TEST_FIT_READER_HPP
