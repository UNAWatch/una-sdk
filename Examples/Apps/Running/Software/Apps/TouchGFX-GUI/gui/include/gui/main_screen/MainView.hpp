#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setGpsFix(bool state);

    void setPositionId(uint16_t id);
    uint16_t getPositionId();

protected:
    using Menu = App::MenuNav::Root;

    bool mGpsFix = false;

    MenuItemConfig mItems[Menu::ID_COUNT] {};
    MenuItemConfig mCenterItems[Menu::ID_COUNT] {};

    touchgfx::Callback<MainView, MainMenuItem&,       int16_t> mUpdateItemCb;
    touchgfx::Callback<MainView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;
    touchgfx::Callback<MainView, int16_t>                      mpAnimationMiddleCb;

    void setupItems();
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);
    void onAnimationMiddle(int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
    void onConfirm();
    void updateBackground(int16_t index);
};

#endif // MAINVIEW_HPP
