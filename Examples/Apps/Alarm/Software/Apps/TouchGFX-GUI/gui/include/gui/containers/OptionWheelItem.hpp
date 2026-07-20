#ifndef OPTIONWHEELITEM_HPP
#define OPTIONWHEELITEM_HPP

#include <gui_generated/containers/OptionWheelItemBase.hpp>
#include <gui/containers/OptionWheelConfig.hpp>

/**
 * @brief Non-centered (surrounding) item in the OptionWheel scroll list.
 *
 * Rendered for every visible item except the one currently in the center.
 * The screen populates it by calling apply() inside its update callback.
 */
class OptionWheelItem : public OptionWheelItemBase
{
public:
    OptionWheelItem();
    virtual ~OptionWheelItem() {}

    virtual void initialize() override;

    /** @brief Apply a configuration to this item (called from the screen's update callback). */
    void apply(const OptionWheelConfig& cfg);

protected:
    // The generated base buffer is only 10 chars, which clips longer labels
    // (e.g. "Beep & Vibrate"); render into this wider one instead.
    static const uint16_t kTextSize = 24;
    touchgfx::Unicode::UnicodeChar mTextBuffer[kTextSize];
};

#endif // OPTIONWHEELITEM_HPP
