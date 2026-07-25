#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

#include <vector>
#include <string>

/**
 * @brief Main screen: a wrapping OrbitMenu with a static "New" face.
 *
 * The orbit wraps through New -> presets -> recents -> New, so New appears as a
 * small neighbour at the list ends. When New is the centred entry the orbit is
 * hidden and a static plus-icon + "New" face is shown instead. A small side
 * ScrollIndicator counts every entry (New included). L1/L2 scroll, R1 selects
 * (New -> Edit, value -> Menu), R2 leaves the app. Title reads TIMER over
 * presets/New and RECENT over recents.
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
    /**
     * @brief Step the selection one place.
     *
     * Value<->value steps animate; crossing the New face (to or from New) is
     * instant for both the orbit and the scroll indicator, so they stay in sync
     * and the 60px centre never renders the letter label "New" (which its
     * digits-only font would draw as "????").
     */
    void moveSelection(bool forward);
    void syncView();     ///< Toggle static-New face vs orbit and set the title.
    void onConfirm();

    std::vector<Item>             mItems;    ///< Parallel to the orbit entries.
    std::vector<std::string>      mLabels;   ///< Stable backing for Entry::label.
    std::vector<OrbitMenu::Entry> mEntries;
};

#endif // MAINVIEW_HPP
