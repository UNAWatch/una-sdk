#include <gui/alert_screen/AlertView.hpp>
#include <SDK/GUI/Button.hpp>
#include <SDK/GUI/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>

// The three options, in wheel order, and their effect mapping.
static const touchgfx::TypedTextId kLabelIds[] = { T_TEXT_BEEP, T_TEXT_VIBRATE,
                                                   T_TEXT_BEEP_AND_VIBRATE };
static const Timer::Effect kEffects[] = { Timer::EFFECT_BEEP,
                                          Timer::EFFECT_VIBRO,
                                          Timer::EFFECT_BEEP_AND_VIBRO };

// Size-only sphere: centre SemiBold 26, neighbours Medium 18, all white and
// centre-aligned. The teal pill behind the centre marks the selection.
static const OrbitTier kAlertTiers[] = {
    { 0.40f,  T_TMP_SEMIBOLD_26_L, SDK::GUI::Color::WHITE },
    { 1.0e9f, T_TMP_MEDIUM_18,     SDK::GUI::Color::WHITE },
};

AlertView::AlertView()
{
}

void AlertView::setupScreen()
{
    AlertViewBase::setupScreen();

    title.set(T_TEXT_TIMER_UC);

    // Side scroll indicator replaces the left buttons.
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    scrollIndicator.setConfig(ScrollIndicator::kSmall);
    scrollIndicator.setCount(kCount);
    scrollIndicator.setActiveId(0);

    orbitMenu.setTiers(kAlertTiers,
                       static_cast<int16_t>(sizeof(kAlertTiers) / sizeof(kAlertTiers[0])));
    // Uniform spacing; the far (+/-2) rows fall outside the box and are clipped
    // so the 3 options never show a duplicate.
    const OrbitMenu::Anchors anchors = { { 0, 22, 87 }, { 58, 22, 87 }, { 98, 22, 87 } };
    orbitMenu.setAnchors(anchors);
    orbitMenu.setHeight(152);
    orbitMenu.setAnimationSteps(4);
    orbitMenu.setCircularMinItems(3);

    OrbitMenu::Entry entries[kCount] = {};
    for (int16_t i = 0; i < kCount; i++) {
        entries[i].labelId = kLabelIds[i];
    }
    orbitMenu.setItems(entries, kCount);
    orbitMenu.setSelected(0);
}

void AlertView::tearDownScreen()
{
    AlertViewBase::tearDownScreen();
}

void AlertView::set(Timer::Effect effect)
{
    for (int16_t i = 0; i < kCount; i++) {
        if (kEffects[i] == effect) {
            orbitMenu.setSelected(i);
            scrollIndicator.setActiveId(static_cast<uint16_t>(i));
            return;
        }
    }
}

void AlertView::handleKeyEvent(uint8_t key)
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
        application().gotoEditScreenNoTransition();
    }
}

void AlertView::confirm()
{
    int16_t idx = orbitMenu.getSelected();
    if (idx < 0 || idx >= kCount) {
        return;
    }
    presenter->setEffect(kEffects[idx]);
    application().gotoMenuScreenNoTransition();
}
