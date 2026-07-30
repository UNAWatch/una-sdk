#ifndef HOMEWIDGET_HPP
#define HOMEWIDGET_HPP

#include <SDK/Kernel/Kernel.hpp>
#include <SDK/Messages/CommandMessages.hpp>
#include <SDK/Messages/MessageGuard.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace SDK {

namespace detail {

/**
 * @brief Copy a UTF-8 string into a fixed buffer, always NUL-terminated.
 *
 * @p src is assumed to be valid UTF-8. On truncation the copy backs off to a
 * character boundary, so a valid @p src is never split mid-sequence. The routine
 * does not validate @p src: malformed input is copied as-is (still bounded and
 * terminated), never rejected.
 * @p dst holds at most @p cap - 1 payload bytes plus the terminator; a @p cap of
 * 0 writes nothing and a null @p src yields an empty string.
 *
 * Internal helper (the @c detail namespace); not part of the stable SDK surface.
 */
inline void copyUtf8(char* dst, std::size_t cap, const char* src)
{
    if (cap == 0) {
        return;
    }
    std::size_t max = cap - 1;                 // reserve the NUL
    std::size_t n   = 0;
    if (src != nullptr) {
        while (src[n] != '\0' && n < max) {
            ++n;
        }
        // Stopped at the limit mid-sequence? Drop the partial trailing char.
        if (src[n] != '\0') {
            while (n > 0 && (static_cast<unsigned char>(src[n]) & 0xC0) == 0x80) {
                --n;
            }
        }
        std::memcpy(dst, src, n);
    }
    dst[n] = '\0';
}

}  // namespace detail

/**
 * @brief Ergonomic wrapper for the home-screen widget a running app pushes.
 *
 * A running app (its Service) can surface a small live element on the kernel's
 * home screen -- a status label and/or a completion bar -- for the duration of
 * an ongoing activity. Bracket it with @ref start / @ref stop; refresh
 * it with an @ref update whenever the app chooses (there is no fixed cadence). The
 * app is identified by its process, so its name and icon come from the .uapp.
 *
 * An update is authoritative: the WidgetShow mask says exactly what the widget
 * shows now; fields not in the mask are hidden. All sends are fire-and-forget.
 * @code
 *   SDK::HomeWidget widget(mKernel);
 *   widget.start();
 *   widget.update(42.0f, "02:35");                               // label + bar
 *   widget.update(SDK::Message::WIDGET_SHOW_PERCENT, 42.0f, ""); // bar only
 *   ...
 *   widget.stop();
 * @endcode
 */
class HomeWidget {
public:
    explicit HomeWidget(const SDK::Kernel& kernel) : mKernel(kernel) {}

    /** @brief Claim the widget slot; the kernel starts showing the element. */
    bool start() { return send<SDK::Message::RequestWidgetStart>(); }

    /** @brief Release the slot; the kernel removes the element. */
    bool stop()  { return send<SDK::Message::RequestWidgetStop>(); }

    /** @brief Show a label and a 0..100 progress bar filled from its start (both fields). */
    bool update(float percent, const char* text)
    {
        return update(SDK::Message::WIDGET_SHOW_TEXT | SDK::Message::WIDGET_SHOW_PERCENT,
                      percent, text);
    }

    /**
     * @brief Integer/double percent forwarders to the (float, text) overload.
     *
     * update(intVar, text) is the most natural percent call an app writes. Without
     * these, int->float and int->uint32_t are both conversion-rank, so the deleted
     * update(uint32_t, const char*) guard below makes such a call ambiguous rather
     * than resolving to the float overload (same for double). These exact matches
     * win over both deleted conversions, while a lone WidgetShow enum and an OR-ed
     * uint32_t mask still exact-match the deleted guards and fail to compile.
     */
    bool update(int percent, const char* text)    { return update(static_cast<float>(percent), text); }
    bool update(double percent, const char* text) { return update(static_cast<float>(percent), text); }

    /**
     * @brief Show exactly the fields in @p shown (a WidgetShow mask).
     *
     * Fields not in @p shown are hidden. @p text is UTF-8 and only used when
     * WIDGET_SHOW_TEXT is set; @p percent only when WIDGET_SHOW_PERCENT is set.
     * Add WIDGET_PROGRESS_FROM_END to pin the fill to the bar's end -- a countdown
     * that empties from the start.
     */
    bool update(uint32_t shown, float percent, const char* text)
    {
        auto msg = SDK::make_msg<SDK::Message::RequestWidgetUpdate>(mKernel);
        if (!msg) {
            return false;
        }
        msg->shown   = shown & SDK::Message::WIDGET_SHOW_ALL;   // never emit stray bits
        // Clamp to the documented range; the >= test also maps NaN and -inf to 0.
        msg->percent = (percent >= 0.0f) ? (percent > 100.0f ? 100.0f : percent) : 0.0f;
        detail::copyUtf8(msg->text, SDK::Message::WIDGET_TEXT_BYTES, text);
        return msg.send();
    }

    /**
     * @brief Reject a WidgetShow mask where a percent value is expected.
     *
     * WidgetShow is an unscoped enum, so it converts implicitly to float. Without
     * these deletions update(WIDGET_SHOW_TEXT, "12:00") would silently bind to
     * update(float, const char*) as percent = 1.0f and show both fields. Deleting
     * the (mask, text) shapes turns that mistake into a compile error. The uint32_t
     * form also catches OR-ed masks, which promote to uint32_t.
     */
    bool update(SDK::Message::WidgetShow, const char*) = delete;
    bool update(uint32_t, const char*) = delete;

private:
    template <typename T>
    bool send()
    {
        auto msg = SDK::make_msg<T>(mKernel);
        return msg && msg.send();
    }

    const SDK::Kernel& mKernel;
};

} // namespace SDK

#endif // HOMEWIDGET_HPP
