#ifndef HOMEWIDGET_HPP
#define HOMEWIDGET_HPP

#include <SDK/Kernel/Kernel.hpp>
#include <SDK/Messages/CommandMessages.hpp>
#include <SDK/Messages/MessageGuard.hpp>

#include <cstddef>
#include <cstring>

namespace SDK {

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

    /** @brief Show a label and a 0..100 progress bar (both fields). */
    bool update(float percent, const char* text)
    {
        return update(SDK::Message::WIDGET_SHOW_TEXT | SDK::Message::WIDGET_SHOW_PERCENT,
                      percent, text);
    }

    /**
     * @brief Show exactly the fields in @p shown (a WidgetShow mask).
     *
     * Fields not in @p shown are hidden. @p text is UTF-8 and only used when
     * WIDGET_SHOW_TEXT is set; @p percent only when WIDGET_SHOW_PERCENT is set.
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
        copyUtf8(msg->text, SDK::Message::WIDGET_TEXT_BYTES, text);
        return msg.send();
    }

private:
    template <typename T>
    bool send()
    {
        auto msg = SDK::make_msg<T>(mKernel);
        return msg && msg.send();
    }

    /**
     * @brief Copy a UTF-8 string into a fixed buffer, NUL-terminated.
     *
     * Never splits a multi-byte sequence: if @p src does not fit, it is truncated
     * back to the last whole character so the buffer always holds valid UTF-8.
     */
    static void copyUtf8(char* dst, std::size_t cap, const char* src)
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

    const SDK::Kernel& mKernel;
};

} // namespace SDK

#endif // HOMEWIDGET_HPP
