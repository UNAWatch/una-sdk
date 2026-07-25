#include <gui/containers/MainMenuCenterItem.hpp>
#include <gui/containers/MenuItemRenderer.hpp>

MainMenuCenterItem::MainMenuCenterItem()
{
}

void MainMenuCenterItem::initialize()
{
    MainMenuCenterItemBase::initialize();
    // Capture the Designer-configured styles so we can restore them when
    // MenuItemConfig leaves msgIdType / tipIdType as TYPED_TEXT_INVALID.
    mDefaultMsgType = labelText.getTypedText().getId();
    mDefaultTipType = tipText.getTypedText().getId();
    if (mDefaultTipType == TYPED_TEXT_INVALID) {
        mDefaultTipType = mDefaultMsgType; // fallback: ensure resizeHeightToCurrentText() works
    }
}

void MainMenuCenterItem::setLayout(const CenterItemLayout &layout)
{
    mpLayout = &layout;
}

const CenterItemLayout &MainMenuCenterItem::activeLayout(const MenuItemConfig &cfg) const
{
    if (cfg.centerLayout != nullptr) { return *cfg.centerLayout; }
    if (mpLayout         != nullptr) { return *mpLayout;         }
    static const CenterItemLayout sDefault;
    return sDefault;
}

void MainMenuCenterItem::apply(const MenuItemConfig &cfg)
{
    const CenterItemLayout &layout = activeLayout(cfg);

    const TypedTextId msgType = (cfg.msgIdType != TYPED_TEXT_INVALID)
        ? cfg.msgIdType : mDefaultMsgType;
    const TypedTextId tipType = (cfg.tipIdType != TYPED_TEXT_INVALID)
        ? cfg.tipIdType : mDefaultTipType;

    MenuItemRenderer::applyText(labelText, labelTextBuffer, LABELTEXT_SIZE,
        cfg.msgId, msgType, cfg.msgText, cfg.msgChar);

    MenuItemRenderer::applyTip(tipText, tipTextBuffer, TIPTEXT_SIZE,
        cfg.tipId, tipType, cfg.tipText, cfg.tipColor, cfg.tipChar);

    switch (cfg.style) {
        case MenuItemConfig::ICON:
            MenuItemRenderer::applyStaticIcon(icon, cfg.iconId);
            break;
        case MenuItemConfig::ICON_EXT:
            MenuItemRenderer::applyDynamicIcon(icon, mDynamicIconId,
                cfg.extIcon, layout.icon.w, layout.icon.h);
            break;
        default:
            break;
    }

    renderStyle(cfg, layout);
}

void MainMenuCenterItem::renderStyle(const MenuItemConfig &cfg,
                                     const CenterItemLayout &layout)
{
    // Reset all optional widgets to Designer defaults before applying the active style.
    // This prevents stale geometry (height/Y set by a previous style) from leaking
    // into the current render -- especially important when containers are reused by
    // the scroll wheel.
    labelText.setVisible(true);
    tipText.setVisible(false);
    icon.setVisible(false);
    toggle.setVisible(false);

    switch (cfg.style) {
        case MenuItemConfig::SIMPLE:
            labelText.setWidth(getWidth());
            labelText.setX(0);
            MenuItemRenderer::centerTextY(labelText, getHeight());
            break;

        case MenuItemConfig::TOGGLE:
            labelText.setWidth(layout.toggle.textW);
            labelText.setX(layout.toggle.textX);
            MenuItemRenderer::centerTextY(labelText, getHeight());
            toggle.setX(layout.toggle.x);
            toggle.setY((getHeight() - toggle.getHeight()) / 2);
            toggle.setState(cfg.toggleState);
            toggle.setVisible(true);
            break;

        case MenuItemConfig::TIP: {
            const int16_t half = getHeight() / 2;

            labelText.setWidth(getWidth());
            labelText.setX(0);
            labelText.resizeHeightToCurrentText();
            labelText.setY((half - labelText.getHeight()) / 2 + layout.tip.msgOffsetY);

            tipText.setWidth(getWidth());
            tipText.setX(0);
            tipText.resizeHeightToCurrentText();
            tipText.setY(half + (half - tipText.getHeight()) / 2 + layout.tip.hintOffsetY);
            tipText.setVisible(true);
            break;
        }

        case MenuItemConfig::INLINE:
            labelText.setWidth(layout.inl.mainW);
            labelText.setX(layout.inl.mainX);
            MenuItemRenderer::centerTextY(labelText, getHeight());
            tipText.setWidth(layout.inl.tipW);
            tipText.setX(layout.inl.tipX);
            MenuItemRenderer::alignBaselineY(labelText, tipText);
            tipText.setVisible(true);
            break;

        case MenuItemConfig::ICON:
        case MenuItemConfig::ICON_EXT:
            if (cfg.iconId != BITMAP_INVALID || cfg.extIcon != nullptr) {
                labelText.setWidth(layout.icon.textW);
                labelText.setX(layout.icon.textX);
                MenuItemRenderer::centerTextY(labelText, getHeight());
                icon.setPosition(layout.icon.x, layout.icon.y,
                                 layout.icon.w, layout.icon.h);
                icon.setVisible(true);
            } else {
                // No icon data -- fall back to simple layout
                labelText.setWidth(getWidth());
                labelText.setX(0);
                MenuItemRenderer::centerTextY(labelText, getHeight());
            }
            break;

        default:
            break;
    }

    labelText.invalidate();
    tipText.invalidate();
    icon.invalidate();
    toggle.invalidate();
}
