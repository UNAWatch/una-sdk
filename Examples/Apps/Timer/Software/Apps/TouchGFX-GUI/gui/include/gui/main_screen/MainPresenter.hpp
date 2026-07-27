#ifndef MAINPRESENTER_HPP
#define MAINPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MainView;

/**
 * @brief Presenter for the Main screen.
 *
 * Feeds the wheel with the model's presets and recents, keeps it in sync on
 * recents changes, and records the chosen timer before navigating on.
 */
class MainPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MainPresenter(MainView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~MainPresenter() {}

    void onRecentsChanged(const std::vector<Timer>& list) override;

    /** @brief Record a selected preset/recent (and its wheel index) to act on. */
    void selectTimer(const Timer& timer, int16_t index);

    /** @brief Seed the edit flow with a default duration for a new timer. */
    void editNew();

    void exitApp();

    /** @brief The timer to re-select on Main, and whether a restore is pending. */
    const Timer& editTimer() const      { return model->getEditTimer(); }
    bool         takeRestoreSelection() { return model->takeRestoreSelection(); }
    int16_t      restoreIndex() const   { return model->restoreIndex(); }

private:
    MainPresenter();

    MainView& view;
};

#endif // MAINPRESENTER_HPP
