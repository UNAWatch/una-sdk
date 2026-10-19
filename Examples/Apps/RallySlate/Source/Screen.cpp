#include "Screen.hpp"
#include "Notices.hpp"
#include "SDK/GUI/Button.hpp"
#include <cstdio>

namespace {
constexpr uint32_t white = 0xffffff, dim = 0xaaaaaa, cyan = 0x55ffff, gold = 0xffff55;
void rect(lv_obj_t* root, int x, int y, int w, int h, uint32_t color, int radius = 0) {
    auto* r = lv_obj_create(root); lv_obj_remove_style_all(r);
    lv_obj_set_pos(r, x, y); lv_obj_set_size(r, w, h);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(r, lv_color_hex(color), 0);
    lv_obj_set_style_radius(r, radius, 0);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_SCROLLABLE);
}
void text(lv_obj_t* root, const char* value, int x, int y, int w, uint32_t color = white) {
    auto* label = lv_label_create(root);
    lv_obj_set_pos(label, x, y); lv_obj_set_width(label, w);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label, value);
}
void digit(lv_obj_t* root, unsigned n, int x, int y, uint32_t color) {
    static constexpr uint8_t masks[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
    const uint8_t bits = masks[n % 10];
    const int xy[7][4] = {{4,0,18,4},{22,4,4,20},{22,28,4,20},{4,48,18,4},
                           {0,28,4,20},{0,4,4,20},{4,24,18,4}};
    for (unsigned i = 0; i < 7; ++i)
        if (bits & (1u << i)) rect(root,x+xy[i][0],y+xy[i][1],xy[i][2],xy[i][3],color,1);
}
void number(lv_obj_t* root, unsigned n, int x, int y, uint32_t color) {
    if (n >= 10) { digit(root,n/10,x,y,color); digit(root,n%10,x+32,y,color); }
    else digit(root,n,x+16,y,color);
}
}

Screen::Screen(Model& model) : mModel(model), mRoot(lv_obj_create(nullptr)) {
    lv_obj_set_style_bg_color(mRoot, lv_color_hex(0), 0);
    lv_obj_set_style_bg_opa(mRoot, LV_OPA_COVER, 0);
    lv_obj_remove_flag(mRoot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(mRoot, &Screen::keyEvent, LV_EVENT_KEY, this);
    if (mModel.corrupt) mMode = Recover;
    mModel.bind(&Screen::refresh, this);
    render();
}
void Screen::keyEvent(lv_event_t* e) {
    static_cast<Screen*>(lv_event_get_user_data(e))->key(static_cast<uint8_t>(lv_event_get_key(e)));
}
void Screen::key(uint8_t code) {
    namespace B = SDK::GUI::Button;
    // A held or repeated press is not a point. Only the watch's click codes act.
    if (code != B::L1 && code != B::L2 && code != B::R1 && code != B::R2) return;
    switch (mMode) {
    case Board:
        if (code == B::L1) mModel.add(0);
        if (code == B::L2) mModel.add(1);
        if (code == B::R1) mModel.undo();
        if (code == B::R2) { mMode = Menu; mChoice = 0; }
        break;
    case Menu:
        if (code == B::L1) mChoice = (mChoice + 4) % 5;
        if (code == B::L2) mChoice = (mChoice + 1) % 5;
        if (code == B::R2) mMode = Board;
        if (code == B::R1) {
            if (mChoice == 0) mMode = Board;
            if (mChoice == 1) mMode = ConfirmRound;
            if (mChoice == 2) mMode = ConfirmTarget;
            if (mChoice == 3) {
                if (mModel.dirty) mMode = UnsavedExit;
                else { mModel.exitApp(); return; }
            }
            if (mChoice == 4) mMode = Licenses;
        }
        break;
    case ConfirmRound: case ConfirmTarget:
        if (code == B::R1) {
            const uint8_t target = mMode == ConfirmTarget ? (mModel.score.targetIndex + 1) % 3 : mModel.score.targetIndex;
            mModel.reset(target); mMode = Board;
        }
        if (code == B::R2) mMode = Menu;
        break;
    case Recover:
        if (code == B::R1) { mModel.reset(0); mMode = Board; }
        if (code == B::R2) { mModel.exitApp(); return; }
        break;
    case UnsavedExit:
        if (code == B::R1) { mModel.exitApp(); return; }
        if (code == B::R2) mMode = Menu;
        break;
    case Licenses:
        if (code == B::R2) mMode = Menu;
        else {
            if (mLicensePanel && (code == B::L1 || code == B::L2))
                lv_obj_scroll_by(mLicensePanel, 0, code == B::L1 ? 80 : -80, LV_ANIM_OFF);
            lv_obj_invalidate(mRoot);
            return;
        }
        break;
    }
    render();
}
void Screen::render() {
    lv_obj_clean(mRoot);
    mLicensePanel = nullptr;
    text(mRoot,"RALLY SLATE",34,22,172,dim);
    char line[48];
    if (mMode == Board) {
        text(mRoot,"A +",18,56,60,cyan); text(mRoot,"UNDO",155,56,65);
        text(mRoot,"A",41,78,62,cyan); text(mRoot,"B",136,78,62,gold);
        number(mRoot,mModel.score.side(0),44,98,cyan);
        number(mRoot,mModel.score.side(1),138,98,gold);
        rect(mRoot,118,115,4,4,dim,1); rect(mRoot,118,131,4,4,dim,1);
        const int winner = mModel.score.winner();
        if (mModel.dirty) std::snprintf(line,sizeof line,"NOT SAVED");
        else if (mModel.recovered) std::snprintf(line,sizeof line,"RECOVERED SAVE");
        else if (winner >= 0) std::snprintf(line,sizeof line,"ROUND %c WINS",'A'+winner);
        else std::snprintf(line,sizeof line,"%u rallies  |  %s",mModel.score.count,mModel.hasSave()?"saved":"new");
        text(mRoot,line,26,154,188,mModel.dirty ? 0xff5555 : white);
        text(mRoot,"B +",18,178,60,gold); text(mRoot,"MENU",155,178,65);
        std::snprintf(line,sizeof line,"First %u, by 2, cap 30",Rally::Score::targets[mModel.score.targetIndex]);
        text(mRoot,line,25,204,190,dim);
    } else if (mMode == Menu) {
        const char* items[] = {"Resume", "New round", "Change target", "Exit", "Licenses"};
        for (unsigned i=0;i<5;++i) {
            if (i==mChoice) rect(mRoot,38,57+22*i,164,21,0x555555,5);
            text(mRoot,items[i],40,60+22*i,160,i==mChoice?gold:white);
        }
        text(mRoot,"L1/L2 move  R1 select",22,180,196,dim);
        text(mRoot,"R2 back",40,202,160,dim);
    } else if (mMode == Licenses) {
        mLicensePanel = lv_obj_create(mRoot);
        lv_obj_remove_style_all(mLicensePanel);
        lv_obj_set_pos(mLicensePanel,30,52); lv_obj_set_size(mLicensePanel,180,134);
        lv_obj_set_scroll_dir(mLicensePanel,LV_DIR_VER);
        auto* label = lv_label_create(mLicensePanel);
        lv_obj_set_width(label,174);
        lv_obj_set_style_text_font(label,&lv_font_montserrat_14,0);
        lv_obj_set_style_text_color(label,lv_color_hex(white),0);
        lv_label_set_text_static(label,rallyLicenseNotices);
        text(mRoot,"L1/L2 scroll  R2 back",22,202,196,dim);
    } else if (mMode == UnsavedExit) {
        text(mRoot,"Points are NOT saved",23,70,194,gold);
        text(mRoot,"Exit and lose changes?",20,100,200);
        text(mRoot,"R1 exit  |  R2 cancel",24,160,192,dim);
    } else if (mMode == Recover) {
        text(mRoot,"Save is unreadable",30,70,180,gold);
        text(mRoot,"Start a new round?",28,100,184);
        text(mRoot,"R1 new  |  R2 exit",28,160,184,dim);
    } else {
        text(mRoot,"Start a new round?",25,75,190,gold);
        if (mMode == ConfirmTarget) {
            const uint8_t index=(mModel.score.targetIndex+1)%3;
            std::snprintf(line,sizeof line,"New target: %u",Rally::Score::targets[index]);
            text(mRoot,line,28,106,184);
        } else text(mRoot,"Current points reset",24,106,192);
        text(mRoot,"R1 confirm",40,151,160,cyan);
        text(mRoot,"R2 cancel",40,178,160,dim);
    }
    lv_obj_invalidate(mRoot);
#ifdef SIMULATOR
    lv_mem_monitor_t memory;
    lv_mem_monitor(&memory);
    std::printf("RALLY mode=%u count=%u heap_free=%u largest=%u used_percent=%u fragmentation=%u\n",
        unsigned(mMode),unsigned(mModel.score.count),unsigned(memory.free_size),
        unsigned(memory.free_biggest_size),unsigned(memory.used_pct),unsigned(memory.frag_pct));
#endif
}
