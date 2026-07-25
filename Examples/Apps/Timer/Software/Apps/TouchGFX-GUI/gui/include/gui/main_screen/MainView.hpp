#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

#include <vector>
#include <string>

/**
 * @brief Main screen: an animated scroll-wheel menu over the timer catalogue.
 *
 * Uses the shared MainMenu component (scroll wheel + side scroll indicator).
 * The wheel lists a "New" entry, the fixed presets, then the recent timers.
 * The centered value is drawn large; the title reads TIMER over presets and
 * RECENT over recents. L1/L2 scroll, R1 selects (New -> Edit, value -> Menu),
 * R2 leaves the app.
 */
class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /** @brief Replace the presets/recents shown and rebuild the wheel. */
    void setLists(const std::vector<Timer>& presets,
                  const std::vector<Timer>& recents);

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    struct Item {
        enum Kind { NEW, VALUE } kind;
        Timer                    timer;
        bool                     isRecent;
    };

    void buildItems(const std::vector<Timer>& presets,
                    const std::vector<Timer>& recents);
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);
    void onAnimationEnded(int16_t index);
    void updateTitle(int16_t index);
    void onConfirm();

    std::vector<Item>            mItems;
    std::vector<std::string>     mLabels;   ///< Backing storage for msgChar.
    std::vector<MenuItemConfig>  mItemCfg;
    std::vector<MenuItemConfig>  mCenterCfg;

    touchgfx::Callback<MainView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MainView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;
    touchgfx::Callback<MainView, int16_t>                      mAnimationEndedCb;
};

#endif // MAINVIEW_HPP
