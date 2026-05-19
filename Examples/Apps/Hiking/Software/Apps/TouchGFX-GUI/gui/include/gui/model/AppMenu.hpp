/**
 ******************************************************************************
 * @file    AppMenu.hpp
 * @date    27-04-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Menu configuration.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef APP_MENU_HPP
#define APP_MENU_HPP

#include <cstdint>
#include "Settings.hpp"

 // =============================================================================
 // App::MenuNav  --  Navigation state with hierarchical position tracking.
 //
 // Usage in Model:
 //   App::MenuNav::Nav mMenu {};
 //   App::MenuNav::Nav& menu() { return mMenu; }
 //
 // Usage in Presenter:
 //   activate()   -- view.setPositionId(model->menu().get());           // root level
 //                  view.setPositionId(model->menu().settings.get());  // sub-level
 //                  model->menu().settings.resetChildren();
 //   deactivate() -- model->menu().set(view.getPositionId());
 //                  model->menu().reset(); // full tree reset (e.g. on idle timeout)
 // =============================================================================
namespace App::MenuNav
{

// -----------------------------------------------------------------------------
// Position<TMenu>
// Stores the selected index for one menu level.
// TMenu must expose ID_DEFAULT as an unscoped enum member.
// -----------------------------------------------------------------------------
template<typename TMenu>
struct Position {
    uint16_t value = static_cast<uint16_t>(TMenu::ID_DEFAULT);

    void     set(uint16_t v) { value = v; }
    uint16_t get() const { return value; }
    void     reset() { value = static_cast<uint16_t>(TMenu::ID_DEFAULT); }
};


// -----------------------------------------------------------------------------
// Menu descriptors
// Each struct describes one menu screen:
//   enum Id     -- ordered item list (ID_COUNT and ID_DEFAULT required)
//   using *     -- aliases to Settings::Alerts for alert value enums
// -----------------------------------------------------------------------------

struct Root {
    enum Id {
        ID_START = 0, ID_SETTINGS,
        ID_COUNT, ID_DEFAULT = ID_START
    };

    struct Settings {
        enum Id {
            ID_ALERTS = 0, ID_AUTO_PAUSE, ID_PHONE_NOTIF,
            ID_COUNT, ID_DEFAULT = ID_ALERTS
        };

        struct Alerts {
            enum Id {
                ID_DISTANCE = 0, ID_TIME,
                ID_COUNT, ID_DEFAULT = ID_DISTANCE
            };

            using Distance = ::Settings::Alerts::Distance;
            using Time     = ::Settings::Alerts::Time;
        };
    };
};


// TrackView is reached from Root::START.
// TrackAction is always an overlay of TrackView, so it lives inside it.
struct TrackView {
    enum Id {
        ID_TRACK1 = 0, ID_TRACK2, ID_TRACK3, ID_TRACK4,
        ID_COUNT, ID_DEFAULT = ID_TRACK1
    };

    struct Action {
        enum Id {
            ID_RESUME = 0, ID_SUMMARY, ID_SAVE, ID_DISCARD,
            ID_COUNT, ID_DEFAULT = ID_RESUME
        };
    };
};


// -----------------------------------------------------------------------------
// Nav
// Hierarchical navigation state. Each node inherits Position<TMenu> to provide
// get() / set() / reset() for its own index, and adds:
//   resetChildren() -- resets all direct child nodes to their defaults
//   reset()         -- resets own position + all children (full subtree)
//
// Leaf nodes are plain Position<TMenu> -- no children, reset() already defined.
// xNav structs are created only for nodes that have children.
// -----------------------------------------------------------------------------
struct Nav : Position<Root> {

    // TrackView node -- Action is a child (entered from any TrackView page)
    struct TrackViewNav : Position<TrackView> {
        Position<TrackView::Action> action;

        void resetChildren() { action.reset(); }
        void reset() { Position<TrackView>::reset(); resetChildren(); }
    };

    // Settings node -- alerts picker is a child
    struct SettingsNav : Position<Root::Settings> {
        Position<Root::Settings::Alerts> alerts;

        void resetChildren() { alerts.reset(); }
        void reset() { Position<Root::Settings>::reset(); resetChildren(); }
    };

    TrackViewNav track;     // TrackView page + nested TrackAction
    SettingsNav  settings;  // Settings menu + nested alerts

    void resetChildren() { track.reset(); settings.reset(); }
    void reset() { Position<Root>::reset(); resetChildren(); }
};

} // namespace App::MenuNav

#endif // APP_MENU_HPP
