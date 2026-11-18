/**
 * @file LvglPool.hpp
 * @brief LVGL's memory pool in the simulator: sized for the host, watched for
 *        running out, and its peak use reported.
 *
 * Simulator only; the LVGL port calls these when built with SIMULATOR.
 *
 * On the watch LVGL allocates from one static pool of LV_MEM_SIZE bytes. LVGL's
 * objects are mostly pointers, so a 64-bit simulator needs about twice the pool
 * the 32-bit watch does: there, install() adds a second pool of the same size.
 * A 32-bit simulator (the Windows build) keeps exactly the watch's pool, so an
 * app that would run out of memory on the watch runs out there too.
 *
 * When the pool runs out, LVGL returns NULL and most of its callers then crash.
 * LVGL reports the failed allocation in an INFO message first (the simulator
 * builds LVGL with LV_LOG_LEVEL_INFO for this, see lv_conf.h); onLog() turns it
 * into a message naming the pool and ends the process with kExitOutOfMemory.
 *
 * reportPeak() writes the pool's peak use to stderr and sim.log when the GUI stops.
 */

#ifndef SDK_SIMULATOR_LVGL_LVGL_POOL_HPP
#define SDK_SIMULATOR_LVGL_LVGL_POOL_HPP

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "lvgl.h"

#include "SDK/Simulator/Automation/Options.hpp"

namespace SDK::Simulator::LvglPool
{

/// True when this build's pointers are wider than the watch's 32 bits.
constexpr bool kWidePointers = sizeof(void*) > 4;

/// Add the 64-bit build's second pool. Call right after lv_init().
inline void install()
{
    if constexpr (kWidePointers) {
        // Static, like LVGL's own pool; lv_deinit() lets go of it again.
        alignas(16) static uint8_t sExtra[LV_MEM_SIZE];
        lv_mem_add_pool(sExtra, sizeof(sExtra));
    }
}

/// What this build's pool holds, for messages.
inline std::string describe()
{
    const unsigned kb = static_cast<unsigned>(LV_MEM_SIZE / 1024U);
    if constexpr (kWidePointers) {
        return std::to_string(kb) + " KB (LV_MEM_SIZE), doubled to " + std::to_string(2 * kb) +
               " KB in this 64-bit build";
    }
    return std::to_string(kb) + " KB (LV_MEM_SIZE), as on the watch";
}

/**
 * @brief Look at one LVGL log message.
 * @return true to drop it: INFO messages are enabled only for this check, so
 *         the log keeps the watch's WARN level.
 */
inline bool onLog(lv_log_level_t level, const char* text)
{
    if (text != nullptr) {
        const char* what = std::strstr(text, "couldn't allocate memory");
        if (what == nullptr) {
            what = std::strstr(text, "couldn't reallocate memory");
        }
        if (what != nullptr) {
            // "couldn't allocate memory (120 bytes)": keep up to the closing bracket.
            std::string message(what);
            const size_t end = message.find(')');
            if (end != std::string::npos) {
                message.resize(end + 1);
            } else {
                while (!message.empty() && (message.back() == '\n' || message.back() == ' ' ||
                                            message.back() == '\t')) {
                    message.pop_back();
                }
            }
            lv_mem_monitor_t mon;
            lv_mem_monitor(&mon);
            Automation::Options::get().report(
                "OUT OF MEMORY: LVGL %s. Its pool is %s; %u B were in use, the largest free block %u B. "
                "Raise LV_MEM_SIZE in the app's GUI compile definitions, or use less LVGL memory.",
                message.c_str(), describe().c_str(),
                static_cast<unsigned>(mon.total_size - mon.free_size),
                static_cast<unsigned>(mon.free_biggest_size));
            std::_Exit(Automation::kExitOutOfMemory);
        }
    }
    return level < LV_LOG_LEVEL_WARN;
}

/// Report the pool's peak use. Call before lv_deinit().
inline void reportPeak()
{
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    const unsigned peak  = static_cast<unsigned>(mon.max_used);
    const unsigned watch = static_cast<unsigned>(LV_MEM_SIZE);
    if constexpr (kWidePointers) {
        Automation::Options::get().report(
            "LVGL pool: peak %u B against the watch's %u B (LV_MEM_SIZE). This is a 64-bit build, "
            "so the peak overstates the watch's; a 32-bit build measures the watch's.",
            peak, watch);
    } else {
        Automation::Options::get().report("LVGL pool: peak %u B of %u B (LV_MEM_SIZE), %u%%.",
                                          peak, watch, watch != 0 ? peak * 100U / watch : 0U);
    }
}

} // namespace SDK::Simulator::LvglPool

#endif // SDK_SIMULATOR_LVGL_LVGL_POOL_HPP
