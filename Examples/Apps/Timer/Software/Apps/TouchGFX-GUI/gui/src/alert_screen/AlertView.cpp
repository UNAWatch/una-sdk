#include <gui/alert_screen/AlertView.hpp>
#include <SDK/GUI/Button.hpp>
#include <SDK/GUI/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>

// The three options, in wheel order, and their effect mapping.
static const char* const  kLabels[]  = { "Beep", "Vibrate", "Beep & Vibrate" };
static const Timer::Effect kEffects[] = { Timer::EFFECT_BEEP,
                                          Timer::EFFECT_VIBRO,
                                          Timer::EFFECT_BEEP_AND_VIBRO };

// Uniform tier: every row is the same size and colour -- the teal pill behind
// the centre is what marks the selection.
static const OrbitTier kAlertTiers[] = {
    { 1.0e9f, T_TMP_SEMIBOLD_30, SDK::GUI::Color::WHITE },
};

AlertView::AlertView()
{
}

void AlertView::setupScreen()
{
    AlertViewBase::setupScreen();

    title.set("TIMER");

    buttons.setL1(Buttons::WHITE);
    buttons.setL2(Buttons::WHITE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    touchgfx::Unicode::snprintf(alertLabelBuffer, ALERTLABEL_SIZE, "Alert");
    alertLabel.setWildcard(alertLabelBuffer);

    orbitMenu.setTiers(kAlertTiers, 1);
    // Uniform spacing; the far (+/-2) rows fall outside the box and are clipped
    // so the 3 options never show a duplicate.
    const OrbitMenu::Anchors anchors = { { 0, 22, 87 }, { 44, 22, 87 }, { 88, 22, 87 } };
    orbitMenu.setAnchors(anchors);
    orbitMenu.setHeight(152);
    orbitMenu.setAnimationSteps(4);
    orbitMenu.setCircularMinItems(3);

    OrbitMenu::Entry entries[kCount] = {};
    for (int16_t i = 0; i < kCount; i++) {
        entries[i].label = kLabels[i];
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
            return;
        }
    }
}

void AlertView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        orbitMenu.selectPrev();
    }
    else if (key == SDK::GUI::Button::L2) {
        orbitMenu.selectNext();
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
