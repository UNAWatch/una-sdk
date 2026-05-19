#ifndef PAUSEINDICATOR_HPP
#define PAUSEINDICATOR_HPP

#include <gui_generated/containers/PauseIndicatorBase.hpp>

/**
 * @brief Overlay indicator shown when a workout is paused.
 *
 * Displays a pause icon alongside an elapsed time in H:MM:SS / MM:SS format.
 *
 * @code
 * pauseIndicator.setTime(7424); // -> "2:03:44"
 * pauseIndicator.setTime(125);  // -> "02:05"
 * @endcode
 */
class PauseIndicator : public PauseIndicatorBase
{
public:
    PauseIndicator();
    virtual ~PauseIndicator() {}

    virtual void initialize() override;

    /**
     * @brief Set the displayed elapsed time.
     *
     * Formats as H:MM:SS when hours > 0, otherwise MM:SS.
     *
     * @param seconds  Total elapsed seconds.
     */
    void setTime(std::time_t seconds);
};

#endif // PAUSEINDICATOR_HPP
