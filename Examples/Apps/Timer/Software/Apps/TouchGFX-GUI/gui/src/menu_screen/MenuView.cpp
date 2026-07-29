#include <gui/menu_screen/MenuView.hpp>
#include <SDK/GUI/Button.hpp>
#include <SDK/GUI/Color.hpp>
#include <touchgfx/Color.hpp>
#include <Timer.hpp>
#include <texts/TextKeysAndLanguages.hpp>

// The three actions, in wheel order, matching Timer::Action.
static const touchgfx::TypedTextId kLabelIds[] = { T_TEXT_START, T_TEXT_EDIT, T_TEXT_DELETE };

// Size-only sphere: centre SemiBold 35, the row below Medium 18, both white and
// centre-aligned. The teal pill behind the centre marks the selection.
static const OrbitTier kMenuTiers[] = {
    { 0.40f,  T_TMP_SEMIBOLD_35_L, SDK::GUI::Color::WHITE },
    { 1.0e9f, T_TMP_MEDIUM_18,     SDK::GUI::Color::WHITE },
};

MenuView::MenuView()
    : mAnimEndedCb(this, &MenuView::onOrbitAnimationEnded)
{
}

void MenuView::setupScreen()
{
    MenuViewBase::setupScreen();

    title.set(T_TEXT_TIMER_UC);

    // Side scroll indicator replaces the left buttons.
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR2(Buttons::WHITE);
    // R1 colour + pill are set per-selection by syncActionVisuals().

    scrollIndicator.setConfig(ScrollIndicator::kSmall);
    scrollIndicator.setCount(kCount);
    scrollIndicator.setActiveId(0);

    orbitMenu.setTiers(kMenuTiers,
                       static_cast<int16_t>(sizeof(kMenuTiers) / sizeof(kMenuTiers[0])));
    // The box top (Y96) clips the row above the selection off its top edge; its
    // slot holds the value label instead, and the far row below clips off the
    // bottom, leaving centre + one below.
    const OrbitMenu::Anchors anchors = { { 0 }, { 58 }, { 98 } };
    orbitMenu.setAnchors(anchors);
    orbitMenu.setHeight(104);
    orbitMenu.setCenterOffset(-28);   // centreLineY 24 -> centre lands at screen y120 (pill).
    orbitMenu.setAnimationSteps(App::Config::kMenuAnimationSteps);
    orbitMenu.setCircularMinItems(3);

    OrbitMenu::Entry entries[kCount] = {};
    for (int16_t i = 0; i < kCount; i++) {
        entries[i].labelId = kLabelIds[i];
    }
    orbitMenu.setItems(entries, kCount);
    orbitMenu.setSelected(0);
    orbitMenu.setAnimationEndedCallback(mAnimEndedCb);
    syncActionVisuals();
}

void MenuView::tearDownScreen()
{
    MenuViewBase::tearDownScreen();
}

void MenuView::setValue(uint16_t durationSec)
{
    touchgfx::Unicode::snprintf(valueTextBuffer, VALUETEXT_SIZE, "%02u:%02u",
                                durationSec / 60u, durationSec % 60u);
    valueText.invalidate();
}

void MenuView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        orbitMenu.selectPrev();
        scrollIndicator.animateToId(
            static_cast<int16_t>(scrollIndicator.getActiveId() - 1), App::Config::kMenuAnimationSteps);
    }
    else if (key == SDK::GUI::Button::L2) {
        orbitMenu.selectNext();
        scrollIndicator.animateToId(
            static_cast<int16_t>(scrollIndicator.getActiveId() + 1), App::Config::kMenuAnimationSteps);
    }
    else if (key == SDK::GUI::Button::R1) {
        confirm();
    }
    else if (key == SDK::GUI::Button::R2) {
        presenter->prepareReturnToMain();   // Main re-selects this timer
        application().gotoMainScreenNoTransition();
    }
}

void MenuView::onOrbitAnimationEnded()
{
    syncActionVisuals();
}

void MenuView::syncActionVisuals()
{
    const int16_t sel = orbitMenu.getSelected();
    const bool onStart = sel == Timer::ACTION_START;

    // A preset cannot be deleted; keep the menu geometry but grey the pill and
    // hide R1 (also ignored in confirm()) while Delete is centred on a preset.
    const bool deleteDisabled =
        (sel == Timer::ACTION_DELETE) && presenter->isEditPreset();

    playIcon.setVisible(onStart);
    playIcon.invalidate();
    buttons.setR1(deleteDisabled ? Buttons::NONE
                                 : (onStart ? Buttons::TEAL : Buttons::WHITE));

    pillPainter.setColor(deleteDisabled
        ? touchgfx::Color::getColorFromRGB(64, 64, 64)   // #404040 -- disabled
        : touchgfx::Color::getColorFromRGB(0, 64, 64));   // teal
    pill.invalidate();
}

void MenuView::confirm()
{
    switch (orbitMenu.getSelected()) {
    case Timer::ACTION_START:
        presenter->startTimer();
        application().gotoRunningScreenNoTransition();
        break;
    case Timer::ACTION_EDIT:
        application().gotoEditScreenNoTransition();
        break;
    case Timer::ACTION_DELETE:
        if (!presenter->isEditPreset()) {   // presets are not deletable
            application().gotoDeletedScreenNoTransition();
        }
        break;
    default:
        break;
    }
}
