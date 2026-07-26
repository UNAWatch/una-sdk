#include <gui/menu_screen/MenuView.hpp>
#include <SDK/GUI/Button.hpp>
#include <SDK/GUI/Color.hpp>
#include <Timer.hpp>
#include <texts/TextKeysAndLanguages.hpp>

// The three actions, in wheel order, matching Timer::Action.
static const char* const kLabels[] = { "Start", "Edit", "Delete" };

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

    title.set("TIMER");

    // Side scroll indicator replaces the left buttons.
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR2(Buttons::WHITE);
    // R1 colour is set per-selection by updateStartVisuals().

    scrollIndicator.setConfig(ScrollIndicator::kSmall);
    scrollIndicator.setCount(kCount);
    scrollIndicator.setActiveId(0);

    orbitMenu.setTiers(kMenuTiers,
                       static_cast<int16_t>(sizeof(kMenuTiers) / sizeof(kMenuTiers[0])));
    // The box top (Y96) clips the row above the selection off its top edge; its
    // slot holds the value label instead, and the far row below clips off the
    // bottom, leaving centre + one below.
    const OrbitMenu::Anchors anchors = { { 0, 22, 87 }, { 58, 22, 87 }, { 98, 22, 87 } };
    orbitMenu.setAnchors(anchors);
    orbitMenu.setHeight(104);
    orbitMenu.setCenterOffset(-28);   // centreLineY 24 -> centre lands at screen y120 (pill).
    orbitMenu.setAnimationSteps(4);
    orbitMenu.setCircularMinItems(3);

    OrbitMenu::Entry entries[kCount] = {};
    for (int16_t i = 0; i < kCount; i++) {
        entries[i].label = kLabels[i];
    }
    orbitMenu.setItems(entries, kCount);
    orbitMenu.setSelected(0);
    orbitMenu.setAnimationEndedCallback(mAnimEndedCb);
    updateStartVisuals();
}

void MenuView::tearDownScreen()
{
    MenuViewBase::tearDownScreen();
}

void MenuView::setValue(uint16_t durationSec)
{
    touchgfx::Unicode::snprintf(valueTextBuffer, VALUETEXT_SIZE, "%02u:%02u",
                                durationSec / 60u, durationSec % 60u);
    valueText.setWildcard(valueTextBuffer);
    valueText.invalidate();
}

void MenuView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        orbitMenu.selectPrev();
        scrollIndicator.animateToId(
            static_cast<int16_t>(scrollIndicator.getActiveId() - 1), 4);
    }
    else if (key == SDK::GUI::Button::L2) {
        orbitMenu.selectNext();
        scrollIndicator.animateToId(
            static_cast<int16_t>(scrollIndicator.getActiveId() + 1), 4);
    }
    else if (key == SDK::GUI::Button::R1) {
        confirm();
    }
    else if (key == SDK::GUI::Button::R2) {
        application().gotoMainScreenNoTransition();
    }
}

void MenuView::onOrbitAnimationEnded()
{
    updateStartVisuals();
}

void MenuView::updateStartVisuals()
{
    const bool onStart = orbitMenu.getSelected() == Timer::ACTION_START;
    playIcon.setVisible(onStart);
    playIcon.invalidate();
    buttons.setR1(onStart ? Buttons::TEAL : Buttons::WHITE);
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
        application().gotoDeletedScreenNoTransition();
        break;
    default:
        break;
    }
}
