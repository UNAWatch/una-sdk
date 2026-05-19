#ifndef MAINMENUITEM_HPP
#define MAINMENUITEM_HPP

#include <gui_generated/containers/MainMenuItemBase.hpp>
#include <gui/containers/MenuItemConfig.hpp>

/**
 * @brief Surrounding item for MainMenu's scroll wheel.
 *
 * Supports display styles defined by MenuItemConfig::Style:
 *   SIMPLE   -- centered labelText only
 *   TIP      -- labelText + smaller hint line below
 *   ICON     -- static bitmap + labelText
 *
 * Note: TOGGLE and ICON_EXT styles gracefully degrade:
 *   - TOGGLE  -> rendered as SIMPLE (toggle is only interactive in the center item)
 *   - ICON_EXT -> rendered as ICON if iconId is set, otherwise as SIMPLE
 *
 * Layout resolution priority:
 *   MenuItemConfig::itemLayout  >  MainMenu::setItemLayout()  >  built-in defaults
 */
class MainMenuItem : public MainMenuItemBase
{
public:
    MainMenuItem();
    virtual ~MainMenuItem() {}

    virtual void initialize() override;

    /**
     * @brief Render this item according to the given configuration.
     * Called from the screen's wheelUpdateItem callback.
     */
    void apply(const MenuItemConfig &cfg);

    /**
     * @brief Set the default geometry layout for this item instance.
     * Called by MainMenu::setItemLayout() on all surrounding wheel buffer items.
     */
    void setLayout(const ItemLayout &layout);

private:
    const ItemLayout *mpLayout = nullptr;

    // Designer defaults -- captured in initialize() from the generated base widgets
    TypedTextId mDefaultMsgType = TYPED_TEXT_INVALID;
    TypedTextId mDefaultTipType = TYPED_TEXT_INVALID;

    const ItemLayout &activeLayout(const MenuItemConfig &cfg) const;
    void              renderStyle(const MenuItemConfig &cfg,
                                  const ItemLayout &layout);
};

#endif // MAINMENUITEM_HPP
