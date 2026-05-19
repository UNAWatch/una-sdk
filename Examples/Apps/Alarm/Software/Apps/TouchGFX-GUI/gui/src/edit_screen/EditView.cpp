#include <gui/edit_screen/EditView.hpp>
#include <gui/common/AlarmLabels.hpp>

EditView::EditView() :
    mRepeatItemCb(this,       &EditView::updateRepeatItem),
    mRepeatCenterItemCb(this, &EditView::updateRepeatCenterItem),
    mEffectItemCb(this,       &EditView::updateEffectItem),
    mEffectCenterItemCb(this, &EditView::updateEffectCenterItem)
{
}

void EditView::setupScreen()
{
    EditViewBase::setupScreen();

    buttons.setL1(Buttons::WHITE);
    buttons.setL2(Buttons::WHITE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    setPosition(Position::HOURS);

    repeatMenu.setUpdateItemCallback(mRepeatItemCb);
    repeatMenu.setUpdateCenterItemCallback(mRepeatCenterItemCb);
    repeatMenu.setNumberOfItems(Alarm::REPEAT_COUNT);
    repeatMenu.invalidate();

    effectMenu.setUpdateItemCallback(mEffectItemCb);
    effectMenu.setUpdateCenterItemCallback(mEffectCenterItemCb);
    effectMenu.setNumberOfItems(Alarm::EFFECT_COUNT);
    effectMenu.invalidate();
}

void EditView::tearDownScreen()
{
    EditViewBase::tearDownScreen();
}

void EditView::set(uint8_t h, uint8_t m, Alarm::Repeat repeat, Alarm::Effect effect)
{
    timeMenu.setTime(h, m);
    repeatMenu.selectItem(static_cast<int16_t>(repeat));
    effectMenu.selectItem(static_cast<int16_t>(effect));
}

void EditView::updateRepeatItem(OptionWheelItem& item, int16_t index)
{
    OptionWheelConfig cfg;
    cfg.msgId = App::Labels::kRepeatLabels[index];
    item.apply(cfg);
}

void EditView::updateRepeatCenterItem(OptionWheelCenterItem& item, int16_t index)
{
    OptionWheelConfig cfg;
    cfg.msgId = App::Labels::kRepeatLabels[index];
    item.apply(cfg);
}

void EditView::updateEffectItem(OptionWheelItem& item, int16_t index)
{
    OptionWheelConfig cfg;
    cfg.msgId = App::Labels::kEffectLabels[index];
    item.apply(cfg);
}

void EditView::updateEffectCenterItem(OptionWheelCenterItem& item, int16_t index)
{
    OptionWheelConfig cfg;
    cfg.msgId = App::Labels::kEffectLabels[index];
    item.apply(cfg);
}

void EditView::setPosition(Position id)
{
    switch (id) {
    case Position::HOURS:
        title.set(T_TEXT_ALARM_UC);
        setActiveName(T_TEXT_HOURS);
        timeMenu.setVisible(true);
        timeMenu.setActiveHours();
        repeatMenu.setVisible(false);
        effectMenu.setVisible(false);
        tick.setVisible(false);
        break;
    case Position::MINUTES:
        title.set(T_TEXT_ALARM_UC);
        setActiveName(T_TEXT_MIN_DOT);
        timeMenu.setVisible(true);
        timeMenu.setActiveMinutes();
        repeatMenu.setVisible(false);
        effectMenu.setVisible(false);
        tick.setVisible(false);
        tick.invalidate();
        break;
    case Position::REPEAT:
        title.set(T_TEXT_ALARM_UC);
        setActiveName(T_TEXT_REPEAT);
        timeMenu.setVisible(false);
        repeatMenu.setVisible(true);
        effectMenu.setVisible(false);
        tick.setVisible(false);
        tick.invalidate();
        break;
    case Position::EFFECT:
        title.set(T_TEXT_ALARM_UC);
        setActiveName(T_TEXT_ALERT);
        timeMenu.setVisible(false);
        repeatMenu.setVisible(false);
        effectMenu.setVisible(true);
        tick.setVisible(true);
        tick.invalidate();
        break;
    default:
        break;
    }

    title.invalidate();
    timeMenu.invalidate();
    repeatMenu.invalidate();
    effectMenu.invalidate();
    tick.invalidate();
}

void EditView::setActiveName(TypedTextId msgId)
{
    Unicode::snprintf(messageTextBuffer, MESSAGETEXT_SIZE, "%s", touchgfx::TypedText(msgId).getText());
    messageText.invalidate();
}

void EditView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        if (mPosition == Position::HOURS || mPosition == Position::MINUTES) {
            timeMenu.decValue();
        } else if (mPosition == Position::REPEAT) {
            repeatMenu.selectPrev();
        } else if (mPosition == Position::EFFECT) {
            effectMenu.selectPrev();
        }
    }

    if (key == SDK::GUI::Button::L2) {
        if (mPosition == Position::HOURS || mPosition == Position::MINUTES) {
            timeMenu.incValue();
        } else if (mPosition == Position::REPEAT) {
            repeatMenu.selectNext();
        } else if (mPosition == Position::EFFECT) {
            effectMenu.selectNext();
        }
    }

    if (key == SDK::GUI::Button::R1) {
        if (mPosition == Position::EFFECT) {
            uint8_t h = 0;
            uint8_t m = 0;
            timeMenu.getTime(h, m);
            presenter->save(h, m,
                static_cast<Alarm::Repeat>(repeatMenu.getSelectedItem()),
                static_cast<Alarm::Effect>(effectMenu.getSelectedItem()));
            application().gotoSavedScreenNoTransition();
        }
        else {
            mPosition = static_cast<Position>(static_cast<uint8_t>(mPosition) + 1);
            setPosition(mPosition);
        }
    }

    if (key == SDK::GUI::Button::R2) {
        if (mPosition == Position::HOURS) {
            presenter->exitScreen();
        }
        else {
            mPosition = static_cast<Position>(static_cast<uint8_t>(mPosition) - 1);
            setPosition(mPosition);
        }
    }
}
