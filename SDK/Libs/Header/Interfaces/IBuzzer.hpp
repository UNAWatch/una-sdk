/**
 ******************************************************************************
 * @file    IBuzzer.hpp
 * @date    14-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Buzzer interface.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __IBUZZER_HPP
#define __IBUZZER_HPP

#include <stdint.h>

namespace Interface
{

/**
 * @brief   Buzzer interface.
 */
class IBuzzer {
public:

    // Maximum Notes includes pauses.
    static const uint8_t skMaxNotes = 10;


    typedef struct {
        uint32_t time;      ///< Duration in ms
        uint8_t  level;     ///< Sound level 1,2,3, 0 - no sound
    } Note_t;

    /**
     * @brief   Play short beep.
     * @retval  Execution status. 'true' - success, 'false' - otherwise.
     */
    virtual bool play() = 0;

    /**
     * @brief   Play note.
     * @note    Commands are queued, so you can call this method without delay.
     * @param   note: Note_t to play;
     * @retval  Execution status. 'true' - success, 'false' - otherwise.
     */
    virtual bool play(Note_t note) = 0;

    /**
     * @brief   Play melody.
     * @param   melody: array of notes.
     * @param   len: length of the array. (Max len is skNoteQueueSize)
     * @retval  Execution status. 'true' - success, 'false' - otherwise.
     */
    virtual bool play(const Note_t melody[], uint8_t len) = 0;

    /**
     * @brief   Check whether the buzzer is playing.
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
    virtual ~IBuzzer() = default;

};

} /* namespace Interface */

#endif /* __IBUZZER_HPP */
