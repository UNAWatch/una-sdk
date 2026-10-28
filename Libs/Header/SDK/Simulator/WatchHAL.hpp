/**
 * @file WatchHAL.hpp
 * @brief TouchGFX simulator HAL that shows the screen inside the UNA Watch.
 *
 * Simulator only. Use it in place of touchgfx::HALSDL2 in simulator/main.cpp:
 *
 *     touchgfx::HAL& hal = touchgfx::touchgfx_generic_init<SDK::Simulator::WatchHAL>(dma, lcd, tc, SIM_WIDTH, SIM_HEIGHT, 0, 0);
 *     setupSimulator(argc, argv, hal);
 *     static_cast<SDK::Simulator::WatchHAL&>(hal).init();
 *     static_cast<touchgfx::HALSDL2&>(hal).setVsyncInterval(SDK::Simulator::WatchHAL::frameIntervalMs(SDK::GUI::Config::kFrameRate));
 *
 * What it adds to HALSDL2:
 *
 * - **Watch housing.** The window shows a UNA Watch render around the 240x240
 *   display (Utilities/Simulator/Skins/landscape_<body>.png, copied next to the
 *   simulator by the app's una/Makefile). The display's pixels outside the round
 *   glass show the skin under them, so the screen sits in the bezel and its
 *   printed labels (UP, DOWN, START / SELECT / STOP, BACK) stay visible.
 *   **F4** cycles white -> black -> teal -> no housing -> white. Without the
 *   housing the corners are black, as on the watch. The housing window has no
 *   title bar: drag it by the housing (or anywhere with the right button).
 *   An app's own simulator/landscape.png (any other image) is left alone: F4
 *   then only toggles it, as in plain TouchGFX.
 * - **Panel look** (L, or UNA_SIM_LOOK=watch): every frame toned the way the
 *   real panel shows it (lifted blue-grey blacks, dimmer whites, about half the
 *   saturation). The app's colours don't change; only the PC window does.
 * - **Slow motion** (UNA_SIM_SLOW=2, factor 1-10, through frameIntervalMs()):
 *   every frame lasts that many times longer, to judge timing the way the watch
 *   feels on frames that redraw a lot.
 *
 * Start options (environment): UNA_SIM_HOUSING=white|black|teal|none,
 * UNA_SIM_LOOK=watch, UNA_SIM_SLOW=<factor>.
 *
 * How TouchGFX skins behave, and why it is done this way (HALSDL2, 4.26.1):
 * a skin with any transparency makes HALSDL2 cut a shaped window, and the
 * shape also cuts the screen hole out of the window, so the skins are fully
 * opaque and this class rounds the display itself. HALSDL2 always loads
 * "landscape.png" from the simulator's folder, so changing body copies
 * landscape_<body>.png over it and reloads; the last body stays until the
 * next build. F4 is handled inside HALSDL2's event loop, so an SDL event
 * filter takes it first.
 */

#pragma once

#include <platform/hal/simulator/sdl2/HALSDL2.hpp>
#include <touchgfx/Application.hpp>
#include <touchgfx/Utils.hpp>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED // keep SDL from renaming main()
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace SDK::Simulator {

class WatchHAL : public touchgfx::HALSDL2
{
public:
    using touchgfx::HALSDL2::HALSDL2;

    /// Call once after setupSimulator(): reads the start options and shows the housing.
    void init()
    {
        const char* look = std::getenv("UNA_SIM_LOOK");
        mPanelLook = (look != nullptr && std::strcmp(look, "watch") == 0);

        // Headless runs (CI: SDL_VIDEODRIVER=dummy) can't make the shaped skin window.
        const char* driver = SDL_GetCurrentVideoDriver();
        if (driver == nullptr || std::strcmp(driver, "dummy") == 0 || std::strcmp(driver, "offscreen") == 0) {
            return;
        }

        // Window tracking (see windowIsLargerThanDisplay) in every mode; F4 is
        // only taken over once the SDK housings are in use (mHousing below).
        mWindowId = initialWindowId();
        SDL_SetEventFilter(&WatchHAL::filterEvents, this);

        char path[320];
        SDL_Surface* probe = IMG_Load(localFileName(path, sizeof(path), bodyFile(0)));
        if (probe == nullptr) {
            touchgfx_printf("WatchHAL: no %s next to the simulator, no watch housing\n", bodyFile(0));
            return;
        }
        SDL_FreeSurface(probe);

        int start = -1;
        if (const char* want = std::getenv("UNA_SIM_HOUSING")) {
            if (std::strcmp(want, "none") == 0) {
                start = kNone;
            }
            for (int i = 0; i < kBodies; ++i) {
                if (std::strcmp(want, kBodyNames[i]) == 0) {
                    start = i;
                }
            }
        }
        if (start < 0) {
            start = currentBody();
            if (start == kCustomSkin) {
                touchgfx_printf("WatchHAL: the app has its own landscape.png, F4 toggles it\n");
                return;
            }
        }

        mHousing = true;
        showBody(start == kNone ? 0 : start);
        if (start == kNone) {
            toggleSkin();
        }
        mBody = start;
        printHelp();
    }

    /// Frame time in ms for setVsyncInterval(): the watch's, times UNA_SIM_SLOW (1-10).
    static float frameIntervalMs(float frameRate)
    {
        float ms = 1000.0f / frameRate;
        if (const char* slow = std::getenv("UNA_SIM_SLOW")) {
            const float factor = static_cast<float>(std::atof(slow));
            if (factor >= 1.0f && factor <= 10.0f) {
                ms *= factor;
            }
        }
        return ms;
    }

    bool sampleKey(uint8_t& key) override
    {
        if (mF4Pending.exchange(false)) {
            nextBody();
        }
        if (!touchgfx::HALSDL2::sampleKey(key)) {
            return false;
        }
        if (key == 'l' || key == 'L') {
            mPanelLook = !mPanelLook;
            touchgfx::Application::getInstance()->invalidate(); // repaint in the new look
            return false;                                       // not an app key
        }
        return true;
    }

protected:
    void renderLCD_FrameBufferToMemory(const touchgfx::Rect& rect, uint8_t* frameBuffer) override
    {
        // 24 bpp, bytes B, G, R (see HALSDL2::scaleTo24bpp), sized like the
        // surface HALSDL2 makes from it. Rebuilt from the 8-bit framebuffer
        // before every call, so changing it in place is safe.
        const int w = DISPLAY_WIDTH;
        const int h = DISPLAY_HEIGHT;
        const float cx = (w - 1) / 2.0f;
        const float cy = (h - 1) / 2.0f;
        const float r2 = kGlassRadius * kGlassRadius;
        const bool skinShown = windowIsLargerThanDisplay();
        makeHousingDraggable();
        static const uint8_t black[3] = { 0, 0, 0 };
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                uint8_t* p = frameBuffer + 3 * (y * w + x);
                const float dx = x - cx;
                const float dy = y - cy;
                if (dx * dx + dy * dy > r2) {
                    // Outside the round glass. Housing: the skin under it, so the
                    // square corners don't hide the bezel. No skin: black, as on
                    // the watch. An app's own skin: untouched.
                    if (skinShown && !mHousing) {
                        continue;
                    }
                    const uint8_t* s = skinShown ? skinPixel(x, y) : black;
                    p[0] = s[2];
                    p[1] = s[1];
                    p[2] = s[0];
                } else if (mPanelLook) {
                    toPanel(p[2], p[1], p[0]);
                }
            }
        }
        // The rect only limits what HALSDL2 copies; widen it so a partial
        // update never leaves an old corner behind.
        (void)rect;
        touchgfx::HALSDL2::renderLCD_FrameBufferToMemory(touchgfx::Rect(0, 0, w, h), frameBuffer);
    }

private:
    // Utilities/Simulator/Skins: display offset printed by make_skins.py (the
    // same for every body), and the screen hole's radius (238 px across).
    static constexpr int   kSkinX = 69;
    static constexpr int   kSkinY = 144;
    static constexpr float kGlassRadius = 119.0f;
    // The bezel right around the screen hole, used if the skin can't be read.
    static constexpr uint8_t kBezel = 14;

    static constexpr int kBodies = 3;
    static constexpr int kNone = kBodies;         // F4 state: no housing
    static constexpr int kCustomSkin = -1;        // landscape.png is the app's own
    static constexpr const char* kBodyNames[kBodies] = { "white", "black", "teal" };

    // Panel look, measured from a video of the real watch (dark clockface,
    // white digits, teal arc): desaturate toward luma, then map 0..255 onto the
    // panel's black..white per channel.
    static constexpr float kSaturation = 0.45f;
    static constexpr float kBlack[3] = { 36.0f, 38.0f, 44.0f };    // R, G, B
    static constexpr float kWhite[3] = { 188.0f, 190.0f, 196.0f };

    // The window setupSimulator() made, before the filter saw any event for it.
    // Only this one lookup scans IDs: at start-up they are still low.
    static Uint32 initialWindowId()
    {
        if (SDL_Window* w = SDL_GetKeyboardFocus()) {
            return SDL_GetWindowID(w);
        }
        for (Uint32 id = 1; id < 64; ++id) {
            if (SDL_Window* w = SDL_GetWindowFromID(id)) {
                return SDL_GetWindowID(w);
            }
        }
        return 0;
    }

    static const char* bodyFile(int body)
    {
        static const char* files[kBodies] = { "landscape_white.png", "landscape_black.png", "landscape_teal.png" };
        return files[body];
    }

    // Every SDL event passes here before HALSDL2's event loop sees it.
    // - Window events: remember the window's ID. HALSDL2 recreates its window
    //   (new ID) on every skin change, and keeps the pointer file-static.
    // - F4 (SDK housings only): taken here and handled on the next tick from
    //   sampleKey(). mPassF4 lets our own F4 (skin on/off) through.
    static int SDLCALL filterEvents(void* self, SDL_Event* event)
    {
        WatchHAL* hal = static_cast<WatchHAL*>(self);
        if (event->type == SDL_WINDOWEVENT) {
            hal->mWindowId = event->window.windowID;
            return 1;
        }
        if (!hal->mHousing || event->type != SDL_KEYUP || event->key.keysym.sym != SDLK_F4) {
            return 1;
        }
        if (hal->mPassF4.exchange(false)) {
            return 1;
        }
        hal->mF4Pending = true;
        return 0;
    }

    void nextBody()
    {
        const int next = (mBody + 1) % (kBodies + 1);
        if (next == kNone) {
            toggleSkin(); // housing off
        } else if (mBody == kNone) {
            showBody(next); // loads with the skin still off...
            toggleSkin();   // ...then turns it on
        } else {
            showBody(next);
        }
        mBody = next;
        touchgfx_printf("WatchHAL: housing %s\n", next == kNone ? "off" : kBodyNames[next]);
    }

    // landscape_<body>.png -> landscape.png, which is the only name HALSDL2 loads.
    void showBody(int body)
    {
        char from[320];
        char to[320];
        if (!copyFile(localFileName(from, sizeof(from), bodyFile(body)),
                      localFileName(to, sizeof(to), "landscape.png"))) {
            touchgfx_printf("WatchHAL: could not copy %s to %s\n", from, to);
            return;
        }
        loadSkin(touchgfx::ORIENTATION_LANDSCAPE, kSkinX, kSkinY);
        if (mSkin != nullptr) {
            SDL_FreeSurface(mSkin);
            mSkin = nullptr;
        }
        mSkinTried = false;
        touchgfx::Application::getInstance()->invalidate();
    }

    // HALSDL2's own F4: skin on/off, window recreated.
    void toggleSkin()
    {
        mPassF4 = true;
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_KEYUP;
        event.key.keysym.sym = SDLK_F4;
        SDL_PushEvent(&event);
    }

    // Which body landscape.png is now (the last one shown), kNone if there is
    // none yet, kCustomSkin if it is some other image.
    int currentBody()
    {
        char path[320];
        SDL_RWops* current = SDL_RWFromFile(localFileName(path, sizeof(path), "landscape.png"), "rb");
        if (current == nullptr) {
            return 0;
        }
        SDL_RWclose(current);
        for (int i = 0; i < kBodies; ++i) {
            char body[320];
            if (sameFile(path, localFileName(body, sizeof(body), bodyFile(i)))) {
                return i;
            }
        }
        return kCustomSkin;
    }

    // Whole file, SDL_free() it. (TouchGFX ships SDL 2.0.4: no SDL_LoadFile.)
    static void* loadFile(const char* path, size_t* size)
    {
        SDL_RWops* in = SDL_RWFromFile(path, "rb");
        if (in == nullptr) {
            return nullptr;
        }
        const Sint64 n = SDL_RWsize(in);
        void* data = (n > 0) ? SDL_malloc(static_cast<size_t>(n)) : nullptr;
        if (data != nullptr && SDL_RWread(in, data, 1, static_cast<size_t>(n)) != static_cast<size_t>(n)) {
            SDL_free(data);
            data = nullptr;
        }
        SDL_RWclose(in);
        *size = (data != nullptr) ? static_cast<size_t>(n) : 0;
        return data;
    }

    static bool sameFile(const char* a, const char* b)
    {
        size_t na = 0;
        size_t nb = 0;
        void* da = loadFile(a, &na);
        void* db = loadFile(b, &nb);
        const bool same = da != nullptr && db != nullptr && na == nb && std::memcmp(da, db, na) == 0;
        SDL_free(da);
        SDL_free(db);
        return same;
    }

    static bool copyFile(const char* from, const char* to)
    {
        size_t size = 0;
        void* data = loadFile(from, &size);
        if (data == nullptr) {
            return false;
        }
        SDL_RWops* out = SDL_RWFromFile(to, "wb");
        const bool ok = out != nullptr && SDL_RWwrite(out, data, 1, size) == size;
        if (out != nullptr) {
            SDL_RWclose(out);
        }
        SDL_free(data);
        return ok;
    }

    // R, G, B of the skin under display pixel (x, y).
    const uint8_t* skinPixel(int x, int y)
    {
        static const uint8_t bezel[3] = { kBezel, kBezel, kBezel };
        if (!mSkinTried) {
            mSkinTried = true;
            char path[320];
            if (SDL_Surface* loaded = IMG_Load(localFileName(path, sizeof(path), "landscape.png"))) {
                mSkin = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGB24, 0);
                SDL_FreeSurface(loaded);
            }
        }
        const int sx = kSkinX + x;
        const int sy = kSkinY + y;
        if (mSkin == nullptr || sx >= mSkin->w || sy >= mSkin->h) {
            return bezel;
        }
        return static_cast<const uint8_t*>(mSkin->pixels) + sy * mSkin->pitch + sx * 3;
    }

    // HALSDL2 keeps its window and skin state private (the window pointer is
    // file-static in HALSDL2.cpp); the window is the display's size exactly
    // when no skin is shown. Mouse focus first, then keyboard focus, then the
    // ID the event filter last saw.
    bool windowIsLargerThanDisplay() const
    {
        SDL_Window* window = SDL_GetMouseFocus();
        if (window == nullptr) {
            window = SDL_GetKeyboardFocus();
        }
        if (window == nullptr && mWindowId != 0) {
            window = SDL_GetWindowFromID(mWindowId);
        }
        int ww = 0;
        int wh = 0;
        if (window != nullptr) {
            SDL_GetWindowSize(window, &ww, &wh);
        }
        return ww > DISPLAY_WIDTH || wh > DISPLAY_HEIGHT;
    }

    // A skinned window has no title bar, so HALSDL2 only moves it with a
    // right-button drag. With an SDK housing, a left drag on the housing (anything
    // outside the round glass) moves it too. Set on each new window: HALSDL2
    // makes one on every skin change. An app's own skin is left alone, since
    // where its display sits isn't known here. The no-housing window gets no hit
    // test: SDL (2.0.4, Windows) answers SDL_HITTEST_NORMAL with HTCLIENT even
    // on the title bar, which would stop it dragging and its buttons working.
    void makeHousingDraggable()
    {
        const Uint32 id = mWindowId;
        if (!mHousing || id == 0 || id == mHitTestWindowId) {
            return;
        }
        SDL_Window* window = SDL_GetWindowFromID(id);
        if (window == nullptr) {
            return;
        }
        mHitTestWindowId = id;
        int ww = 0;
        int wh = 0;
        SDL_GetWindowSize(window, &ww, &wh);
        if (ww > DISPLAY_WIDTH || wh > DISPLAY_HEIGHT) {
            SDL_SetWindowHitTest(window, &WatchHAL::hitTest, nullptr);
        }
    }

    // Only on a window with the housing (see makeHousingDraggable).
    static SDL_HitTestResult SDLCALL hitTest(SDL_Window*, const SDL_Point* p, void*)
    {
        const float dx = p->x - (kSkinX + (DISPLAY_WIDTH - 1) / 2.0f);
        const float dy = p->y - (kSkinY + (DISPLAY_HEIGHT - 1) / 2.0f);
        return (dx * dx + dy * dy > kGlassRadius * kGlassRadius) ? SDL_HITTEST_DRAGGABLE : SDL_HITTEST_NORMAL;
    }

    static void toPanel(uint8_t& r, uint8_t& g, uint8_t& b)
    {
        const float in[3] = { static_cast<float>(r), static_cast<float>(g), static_cast<float>(b) };
        const float luma = 0.299f * in[0] + 0.587f * in[1] + 0.114f * in[2];
        uint8_t* out[3] = { &r, &g, &b };
        for (int c = 0; c < 3; ++c) {
            float v = luma + kSaturation * (in[c] - luma);
            v = (v < 0.0f) ? 0.0f : (v > 255.0f ? 255.0f : v);
            *out[c] = static_cast<uint8_t>(kBlack[c] + (kWhite[c] - kBlack[c]) * v / 255.0f + 0.5f);
        }
    }

    static void printHelp()
    {
        touchgfx_printf("WatchHAL: F4 = next watch housing (white, black, teal, none), L = panel colours, F3 = screenshot, drag the housing to move\n");
    }

    bool              mHousing = false;  // SDK skins in use, F4 cycles them
    int               mBody = 0;         // 0..kBodies-1, or kNone
    bool              mPanelLook = false;
    std::atomic<bool> mF4Pending{ false };
    std::atomic<bool> mPassF4{ false };
    std::atomic<Uint32> mWindowId{ 0 };  // HALSDL2's current window, from window events
    Uint32            mHitTestWindowId = 0; // the window makeHousingDraggable() last set up
    bool              mSkinTried = false;
    SDL_Surface*      mSkin = nullptr;   // the shown skin, for the display's corners
};

} // namespace SDK::Simulator
