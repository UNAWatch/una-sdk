#ifndef OPTIONWHEELCONFIG_HPP
#define OPTIONWHEELCONFIG_HPP

#include <texts/TextKeysAndLanguages.hpp>

/**
 * @brief Configuration passed to OptionWheelItem / OptionWheelCenterItem via apply().
 *
 * The screen fills this struct inside its wheel update callbacks and hands it
 * to the item. The wheel itself remains stateless -- all data lives in the screen.
 *
 * Style::SIMPLE  -- a single text label (msgId).
 * Style::TOGGLE  -- two side labels (msgIdLeft / msgIdRight) plus a toggle switch.
 */
struct OptionWheelConfig
{
    enum Style {
        SIMPLE = 0,  ///< Plain text label
        TOGGLE       ///< Left/right labels with a toggle switch
    };

    Style       style       = SIMPLE;
    TypedTextId msgId       = TYPED_TEXT_INVALID;  ///< Label text             (SIMPLE)
    TypedTextId msgIdLeft   = TYPED_TEXT_INVALID;  ///< Left label             (TOGGLE)
    TypedTextId msgIdRight  = TYPED_TEXT_INVALID;  ///< Right label            (TOGGLE)
    bool        toggleState = false;               ///< Current switch state   (TOGGLE)
};

#endif // OPTIONWHEELCONFIG_HPP
