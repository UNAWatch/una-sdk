#include "Model.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

namespace { constexpr const char* paths[] = {"rally-a.bin", "rally-b.bin"}; }

Model::Model() : mKernel(SDK::KernelProviderGUI::GetInstance().getKernel()) {
    load();
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);
}

void Model::load() {
    Rally::Score scores[2]; uint32_t serials[2]{};
    bool exists[2]{}, valid[2]{};
    for (int i = 0; i < 2; ++i) {
        auto file = mKernel.fs.file(paths[i]);
        exists[i] = file && file->exist();
        if (!exists[i] || !file->open(false, false)) continue;
        Rally::Record record{}; size_t read = 0;
        const bool sizeOK = file->size() == record.size();
        const bool readOK = file->read(reinterpret_cast<char*>(record.data()), record.size(), read);
        const bool closeOK = file->close();
        valid[i] = sizeOK && readOK && closeOK && read == record.size() &&
                   Rally::decode(record, scores[i], serials[i]);
    }
    if (!valid[0] && !valid[1]) {
        corrupt = exists[0] || exists[1];
        return;
    }
    mSlot = !valid[0] ? 1 : (!valid[1] ? 0 : (Rally::newer(serials[1], serials[0]) ? 1 : 0));
    mSerial = serials[mSlot]; score = scores[mSlot];
    recovered = (exists[0] && !valid[0]) || (exists[1] && !valid[1]);
}

void Model::save() {
    const int slot = mSlot == 0 ? 1 : 0;
    const uint32_t serial = mSerial + 1;
    const auto record = Rally::encode(score, serial);
    dirty = true;
    auto file = mKernel.fs.file(paths[slot]);
    if (!file || !file->open(true, true)) return;
    size_t written = 0;
    bool ok = file->write(reinterpret_cast<const char*>(record.data()), record.size(), written);
    ok = written == record.size() && ok;
    ok = file->flush() && ok;
    ok = file->close() && ok;
    if (ok) { mSlot = slot; mSerial = serial; dirty = false; recovered = false; }
}

bool Model::add(unsigned side) {
    if (corrupt || !score.add(side)) return false;
    save(); return true;
}
bool Model::undo() {
    if (corrupt || !score.undo()) return false;
    save(); return true;
}
void Model::reset(uint8_t target) {
    score.reset(target); corrupt = false; save();
}
void Model::exitApp() {
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    mKernel.sys.exit();
}
