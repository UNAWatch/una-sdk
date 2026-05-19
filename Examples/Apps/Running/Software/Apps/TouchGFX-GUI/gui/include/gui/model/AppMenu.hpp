/**
 ******************************************************************************
 * @file    AppMenu.hpp
 * @date    17-04-2026
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
//                  view.setPositionId(model->menu().intervals.get());  // sub-level
//                  model->menu().intervals.resetChildren();
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
    uint16_t get() const     { return value; }
    void     reset()         { value = static_cast<uint16_t>(TMenu::ID_DEFAULT); }
};


// -----------------------------------------------------------------------------
// Menu descriptors
// Each struct describes one menu screen:
//   enum Id     -- ordered item list (ID_COUNT and ID_DEFAULT required)
//   kValues[]   -- optional: domain values mapped 1-to-1 to enum items
// -----------------------------------------------------------------------------

struct Root {
    enum Id { ID_START = 0, ID_INTERVALS, ID_SETTINGS,
              ID_COUNT, ID_DEFAULT = ID_START };

    struct Intervals {
        enum Id {
            ID_START = 0, ID_REPEATS, ID_RUN, ID_REST,
            ID_WARM_UP, ID_COOL_DOWN,
            ID_COUNT, ID_DEFAULT = ID_START
        };

        // Dynamic menu: Open + x1..x20 -- no fixed enum, items built at runtime
        struct Repeats {
            static constexpr uint16_t kMaxRepeats = 20;          // x1..x20
            static constexpr uint16_t kMaxCount   = kMaxRepeats + 1; // + Open
            static constexpr uint16_t ID_DEFAULT  = 0;           // Open
        };

        // For RUN, REST submenus
        struct Metric {
            enum Id {
                ID_TIME = 0, ID_DISTANCE, ID_OPEN,
                ID_COUNT, ID_DEFAULT = ID_TIME
            };
        };

        // 2-part time picker (shared by RunTime / RestTime screens)
        // Items are built at runtime -- no fixed enum.
        struct TimePicker {
            static constexpr uint16_t kMaxMin   = 5;    ///< Max minutes: 0..5
            static constexpr uint16_t kStepMin  = 1;
            static constexpr uint16_t kCountMin = kMaxMin / kStepMin + 1; // 6

            static constexpr uint16_t kMaxSec   = 55;   ///< Max seconds: 0..55 (step 5)
            static constexpr uint16_t kStepSec  = 5;
            static constexpr uint16_t kCountSec = kMaxSec / kStepSec + 1; // 12
        };

        // 2-part distance picker (shared by RunDistance / RestDistance screens)
        // Stage WHOLE: pick whole km (0..10) or mi (0..6)
        // Stage FRAC:  pick decimal in 0.05-unit steps (0.00..0.95 -> 20 items)
        struct DistancePicker {
            static constexpr uint16_t kMaxWholeKm   = 10;
            static constexpr uint16_t kMaxWholeMi   = 6;
            static constexpr uint16_t kCountWholeKm = kMaxWholeKm + 1;  ///< 0..10 -> 11 items
            static constexpr uint16_t kCountWholeMi = kMaxWholeMi + 1;  ///< 0..6  -> 7 items

            static constexpr float    kFracStep  = 0.05f;  ///< Units (km or mi) per index step
            static constexpr uint16_t kCountFrac = 20;     ///< 0.00..0.95 in 0.05 steps
        };
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


// TrackView is a destination reached from multiple places (Root::START,
// Root::Intervals::START), so it is not nested inside Root.
// TrackAction is always an overlay of TrackView, so it lives inside it.
struct TrackView {
    enum Id { ID_INTERVALS = 0, ID_TRACK1, ID_TRACK2, ID_TRACK3,
              ID_COUNT, ID_DEFAULT = ID_INTERVALS };

    struct Action {
        enum Id { ID_RESUME = 0, ID_SUMMARY, ID_SAVE, ID_DISCARD,
                  ID_COUNT, ID_DEFAULT = ID_RESUME };
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
        void reset()         { Position<TrackView>::reset(); resetChildren(); }
    };

    // Intervals node -- repeats picker and run/rest metric pickers are children
    struct IntervalsNav : Position<Root::Intervals> {
        Position<Root::Intervals::Repeats> repeats;
        Position<Root::Intervals::Metric>  run;
        Position<Root::Intervals::Metric>  rest;

        void resetChildren() { repeats.reset(); run.reset(); rest.reset(); }
        void reset()         { Position<Root::Intervals>::reset(); resetChildren(); }
    };

    // Settings node -- alerts picker is a child
    struct SettingsNav : Position<Root::Settings> {
        Position<Root::Settings::Alerts> alerts;

        void resetChildren() { alerts.reset(); }
        void reset()         { Position<Root::Settings>::reset(); resetChildren(); }
    };

    TrackViewNav  track;      // TrackView page + nested TrackAction
    IntervalsNav  intervals;  // Intervals menu + nested run/rest metrics
    SettingsNav   settings;   // Settings menu + nested alerts

    void resetChildren() { track.reset(); intervals.reset(); settings.reset(); }
    void reset()         { Position<Root>::reset(); resetChildren(); }
};

} // namespace App::MenuNav

#endif // APP_MENU_HPP
