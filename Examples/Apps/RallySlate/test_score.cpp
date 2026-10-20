#include "Score.hpp"
#include <cassert>
#include <iostream>

int main() {
    using namespace Rally;
    Score s;
    for (int i = 0; i < 10; ++i) assert(s.add(0));
    assert(s.winner() == -1);
    assert(s.add(0) && s.winner() == 0);
    assert(!s.add(1));
    assert(s.undo() && s.winner() == -1 && s.side(0) == 10);
    for (int i = 0; i < 10; ++i) assert(s.undo());
    assert(!s.undo() && s.count == 0);
    for (uint8_t target = 0; target < 3; ++target) {
        s.reset(target);
        for (unsigned i = 0; i < 29; ++i) {
            assert(s.add(0) && s.winner() == -1);
            assert(s.add(1) && s.winner() == -1);
        }
        assert(s.add(1) && s.winner() == 1);
        assert(s.count == 59 && s.side(0) == 29 && s.side(1) == 30);
        for (unsigned i = 0; i < 59; ++i) assert(s.undo());
        assert(s.count == 0 && s.winner() == -1);
    }
    s.reset(2);
    for (unsigned i = 0; i < 20; ++i) assert(s.add(i % 2));
    const Record original = encode(s, 0xffffffffu);
    Score decoded; uint32_t serial = 0;
    assert(decode(original, decoded, serial));
    assert(decoded.points == s.points && decoded.targetIndex == 2 && serial == 0xffffffffu);
    for (size_t i = 0; i < original.size(); ++i) for (unsigned bit = 0; bit < 8; ++bit) {
        Record r = original; r[i] ^= uint8_t(1u << bit);
        assert(!decode(r, decoded, serial));
    }
    for (int bad : {0, 1, 2, 3}) {
        Record r = original;
        if (bad == 0) r[5] = 3;
        if (bad == 1) r[6] = 60;
        if (bad == 2) r[12] = 2;
        if (bad == 3) r[71] = 1;
        put32(r, 72, checksum(r));
        assert(!decode(r, decoded, serial));
    }
    Score finished;
    for (unsigned i = 0; i < 11; ++i) assert(finished.add(0));
    Record bad = encode(finished, 7); bad[6] = 12; bad[23] = 1;
    put32(bad, 72, checksum(bad));
    assert(!decode(bad, decoded, serial));
    assert(newer(0, 0xffffffffu) && !newer(0xffffffffu, 0) && !newer(9, 9));
    assert(!s.add(2));
    std::cout << "PASS target/deuce/cap/undo, roundtrip, 608 damaged records, semantic rejection, sequence wrap\n";
}
