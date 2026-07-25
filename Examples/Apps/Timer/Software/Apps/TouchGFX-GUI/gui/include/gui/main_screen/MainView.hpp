#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

#include <vector>

/**
 * @brief Main screen: a button-driven wheel over the timer catalogue.
 *
 * The wheel lists, in order: a "New" entry, the fixed presets, and -- when any
 * exist -- the recent timers behind a non-selectable "Recent" divider. The
 * centered entry is drawn large; its neighbours are dimmed. The screen title
 * reads TIMER over presets and RECENT over recents.
 *
 * L1/L2 scroll, R1 selects (New -> Edit, a value -> Menu), R2 leaves the app.
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
        enum Kind { NEW, VALUE, DIVIDER } kind;
        Timer                            timer;
        bool                             isRecent;
    };

    void   rebuildItems(const std::vector<Timer>& presets,
                        const std::vector<Timer>& recents);
    void   show();
    size_t step(size_t index, bool forward) const;   ///< Skips the divider.
    void   fillLabel(touchgfx::TextAreaWithOneWildcard& area,
                     touchgfx::Unicode::UnicodeChar* buffer, uint16_t bufSize,
                     size_t index);

    std::vector<Item> mItems;
    size_t            mIndex = 0;
};

#endif // MAINVIEW_HPP
