#pragma once
#include "Model.hpp"
#include "lvgl.h"

class Screen {
public:
    explicit Screen(Model& model);
    lv_obj_t* root() const { return mRoot; }
    void render();
private:
    enum Mode { Board, Menu, ConfirmRound, ConfirmTarget, Recover, UnsavedExit, Licenses };
    Model& mModel;
    lv_obj_t* mRoot;
    lv_obj_t* mLicensePanel = nullptr;
    Mode mMode = Board;
    unsigned mChoice = 0;
    void key(uint8_t code);
    static void keyEvent(lv_event_t* event);
    static void refresh(void* context) { static_cast<Screen*>(context)->render(); }
};
