#ifndef OPTIONWHEELCENTERITEM_HPP
#define OPTIONWHEELCENTERITEM_HPP

#include <gui_generated/containers/OptionWheelCenterItemBase.hpp>
#include <gui/containers/OptionWheelConfig.hpp>

/**
 * @brief Center (selected) item in the OptionWheel scroll list.
 *
 * Rendered only for the item currently in focus. Supports two visual styles:
 *   - SIMPLE : a single highlighted text label.
 *   - TOGGLE : left/right labels flanking a toggle switch.
 *
 * The screen populates it by calling apply() inside its center-item update callback.
 */
class OptionWheelCenterItem : public OptionWheelCenterItemBase
{
public:
    OptionWheelCenterItem();
    virtual ~OptionWheelCenterItem() {}

    virtual void initialize() override;

    /** @brief Apply a configuration to this item (called from the screen's update callback). */
    void apply(const OptionWheelConfig& cfg);
};

#endif // OPTIONWHEELCENTERITEM_HPP
