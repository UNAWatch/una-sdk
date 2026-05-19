#ifndef MAINMENUCENTERITEM_HPP
#define MAINMENUCENTERITEM_HPP

#include <gui_generated/containers/MainMenuCenterItemBase.hpp>
#include <gui/containers/MenuItemConfig.hpp>

/**
 * @brief Center item for MainMenu's scroll wheel.
 *
 * Supports all display styles defined by MenuItemConfig::Style:
 *   SIMPLE   -- centered labelText only
 *   TOGGLE   -- labelText + interactive toggle switch
 *   TIP      -- labelText + smaller hint line below
 *   ICON     -- static bitmap + labelText
 *   ICON_EXT -- dynamic (external raw pixels) bitmap + labelText
 *
 * Layout resolution priority:
 *   MenuItemConfig::centerLayout  >  MainMenu::setCenterItemLayout()  >  built-in defaults
 */
class MainMenuCenterItem : public MainMenuCenterItemBase
{
public:
    MainMenuCenterItem();
    virtual ~MainMenuCenterItem() {}

    virtual void initialize() override;

    /**
     * @brief Render this item according to the given configuration.
     * Called from the screen's wheelUpdateCenterItem callback.
     */
    void apply(const MenuItemConfig &cfg);

    /**
     * @brief Set the default geometry layout for this item instance.
     * Called by MainMenu::setCenterItemLayout() on all center wheel buffer items.
     */
    void setLayout(const CenterItemLayout &layout);

private:
    const CenterItemLayout *mpLayout       = nullptr;
    BitmapId                mDynamicIconId = BITMAP_INVALID;

    // Designer defaults -- captured in initialize() from the generated base widgets
    TypedTextId mDefaultMsgType = TYPED_TEXT_INVALID;
    TypedTextId mDefaultTipType = TYPED_TEXT_INVALID;

    const CenterItemLayout &activeLayout(const MenuItemConfig &cfg) const;
    void                    renderStyle(const MenuItemConfig &cfg,
                                        const CenterItemLayout &layout);
};

#endif // MAINMENUCENTERITEM_HPP
