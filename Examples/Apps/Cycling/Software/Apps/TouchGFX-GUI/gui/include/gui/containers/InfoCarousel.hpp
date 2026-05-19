#ifndef INFOCAROUSEL_HPP
#define INFOCAROUSEL_HPP

#include <gui_generated/containers/InfoCarouselBase.hpp>
#include <touchgfx/Callback.hpp>

/**
 * @brief Cycling info panel that periodically rotates through a set of data slots.
 *
 * The component registers itself as a TouchGFX timer widget lazily -- only when
 * count > 1 -- and unregisters in the destructor. This is safe because the
 * Application outlives all screens.
 *
 * Typical setup in View::setupScreen():
 * @code
 *   infoCarousel.setPeriod(3000);
 *   infoCarousel.setUpdateCallback(mCarouselCb);
 *   infoCarousel.setCount(5);   // registers timer, fires cb(0) immediately
 * @endcode
 *
 * Inside the callback:
 * @code
 *   void MyView::onCarouselUpdate(int16_t index) {
 *       infoCarousel.setTitle(T_TEXT_SOME_LABEL);
 *       infoCarousel.setValue(myBuffer);
 *   }
 * @endcode
 */
class InfoCarousel : public InfoCarouselBase
{
public:
    InfoCarousel();
    virtual ~InfoCarousel();

    virtual void initialize();

    // ---- Cycling control ---------------------------------------------------

    /**
     * @brief Set the number of slots to cycle through.
     * Lazily registers the timer widget when count > 1, unregisters when <= 1.
     * Resets the index to 0 and fires the callback immediately.
     * @param count Number of slots (0 or 1 disables cycling).
     */
    void setCount(uint16_t count);

    /**
     * @brief Set the number of ticks between slot advances.
     * @param ticks Must be > 0.
     */
    void setPeriod(uint16_t ticks);

    /**
     * @brief Register the update callback (must be called before setCount).
     * Signature: void onUpdate(int16_t index).
     */
    void setUpdateCallback(touchgfx::GenericCallback<int16_t>& cb);

    /**
     * @brief Re-fire the callback for the current slot index.
     * Call this whenever the underlying data changes so the visible slot
     * stays up to date.
     */
    void refresh();

    // ---- Content (call from inside the update callback) --------------------

    /** @brief Set the label text from a typed-text ID. */
    void setTitle(TypedTextId msgId);

    /**
     * @brief Set the dataValue text. Pass nullptr to hide the dataValue field.
     * @param messageText Null-terminated Unicode string (may be nullptr).
     */
    void setValue(const touchgfx::Unicode::UnicodeChar* messageText);

    virtual void handleTickEvent() override;

protected:
    uint16_t mCount  = 0;
    uint16_t mPeriod = 1;
    uint16_t mIndex  = 0;
    uint16_t mTick   = 0;
    bool     mRegistered = false;

    touchgfx::GenericCallback<int16_t>* mpUpdateCb = nullptr;

private:
    void registerTimer();
    void unregisterTimer();
    void fireCallback();
};

#endif // INFOCAROUSEL_HPP
