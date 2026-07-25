#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

#include <vector>
#include <string>

/**
 * @brief Main screen: static "New" entry + a COROS-style OrbitMenu of values.
 *
 * Index 0 is a separate static plus-icon + "New" screen (the orbit is hidden);
 * scrolling reveals the OrbitMenu of preset/recent values, which curve and
 * shrink toward a large centred value. The side ScrollIndicator (small style)
 * counts every entry including New. L1/L2 scroll, R1 selects (New -> Edit,
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
    struct Value {
        Timer timer;
        bool  isRecent;
    };

    void buildEntries(const std::vector<Timer>& presets,
                      const std::vector<Timer>& recents);
    void moveSelection(bool forward);
    void syncView();     ///< Toggle static-New vs orbit and set the title.
    void onConfirm();

    std::vector<Value>            mValues;   ///< Orbit entries (New is index 0, separate).
    std::vector<std::string>      mLabels;   ///< Stable backing for Entry::label.
    std::vector<OrbitMenu::Entry> mEntries;

    int16_t mSelIndex = 0;   ///< 0 = New (static); 1.. = value (orbit index - 1).
    int16_t mCount    = 1;   ///< New + values.
};

#endif // MAINVIEW_HPP
