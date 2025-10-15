
#pragma once

#include <cstdint>
#include <cstdbool>

namespace SDK::Interface {

/**
 * @brief   Vibro motor interface.
 */
class IVibro {
public:

    enum Effect {
        NO_EFFECT                            = 0,  // silent
        STRONG_CLICK_100                     = 1,
        SHARP_CLICK_100                      = 4,
        SOFT_BUMP_100                        = 7,
        DOUBLE_CLICK_100                     = 10,
        STRONG_BUZZ_100                      = 14,
        ALERT_750MS_100                      = 15,
        ALERT_1000MS_100                     = 16,
        STRONG_CLICK_1_100                   = 17,
        MEDIUM_CLICK_1_100                   = 21,
        SHARP_TICK_1_100                     = 24,
        SHORT_DOUBLE_CLICK_STRONG_1_100      = 27,
        SHORT_DOUBLE_CLICK_MEDIUM_1_100      = 31,
        SHORT_DOUBLE_SHARP_TICK_1_100        = 34,
        LONG_DOUBLE_SHARP_CLICK_STRONG_1_100 = 37,
        LONG_DOUBLE_SHARP_CLICK_MEDIUM_1_100 = 41,
        LONG_DOUBLE_SHARP_TICK_1_100         = 44,
        BUZZ_1_100                           = 47,
        PULSING_STRONG_1_100                 = 52,
        PULSING_MEDIUM_1_100                 = 54,
        PULSING_SHARP_1_100                  = 56,
    };

    // Maximum Notes includes pauses.
    static const uint8_t skMaxNotes = 8;

    struct Note {
        uint8_t  effect;    // 1 - 127,  0 - for pause
        uint8_t  loop;      // 1 - 3,    0 - no repeat (only for effect)
        uint32_t pause;     // 1 - 1270, 0 - for effect. In ms. Step 10 ms.
    };

    /**
     * @brief   Play effect.
     * @note    Turn on vibro before play.
     * @note    Commands are queued, so you can call this method without delay.
     * @param   effect: Effect to play;
     */
    virtual void play(uint8_t effect = Effect::STRONG_CLICK_100) = 0;

    /**
     * @brief   Play melody.
     * @note    Turn on vibro before play.
     * @param   melody: array of effects. Maximum 8 notes includes pauses.
     * @param   len: length of the array. (Max len is skMaxNotes)
     * @param   loop: main loop 1-6 times, 0 - no repeat, 7 - infinity loop.
     */
    virtual void play(const Note melody[], uint8_t len, uint8_t loop = 0) = 0;

    /**
     * @brief   Check whether the vibro is playing.
     * @retval  'true' if playing, 'false' otherwise.
     */
    virtual bool isPlaying() = 0;

    /**
     * @brief   Stop playing melody.
     */
    virtual void stop() = 0;

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~IVibro() = default;

};

} // namespace SDK::Interface
