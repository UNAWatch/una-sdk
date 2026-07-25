#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

#include <vector>
#include <string>

/**
 * @brief Main screen: a COROS-style OrbitMenu over the timer catalogue.
 *
 * The orbit lists a "New" entry (plus icon), the fixed presets, then the recent
 * timers -- New is a normal element of the wheel. The centered entry is largest
 * and rows shrink/curve toward it. A small side ScrollIndicator (covering every
 * element, New included) replaces the left buttons. The title reads TIMER over
 * presets and RECENT over recents. L1/L2 scroll, R1 selects (New -> Edit,
 * value -> Menu), R2 leaves the app.
 */
class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /** @brief Replace the presets/recents shown and rebuild the orbit. */
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

    void buildEntries(const std::vector<Timer>& presets,
                      const std::vector<Timer>& recents);
    void updateTitle(int16_t index);
    void onConfirm();

    std::vector<Item>            mItems;
    std::vector<std::string>     mLabels;    ///< Stable backing for Entry::label.
    std::vector<OrbitMenu::Entry> mEntries;
};

#endif // MAINVIEW_HPP
