# ClockfaceConsole - Console Clockface

An oversized day of month between the weekday and the month, over a monospaced
clock and the day's step count. The leanest of the five.

![Console](../assets/clockface-console.png)

*Console in the app simulator. The step count is the design's sample value, fed in by hand: the simulator serves STEP_COUNTER rather than STEP_COUNTER_DAILY, so it reads zero when you run it yourself.*

> New to faces? Start with **[Writing a Watch Face](../writing-a-clockface.md)**,
> which covers the structure, the platform constraints and the techniques all
> five faces share. This page is only what is particular to Console.

| | |
|---|---|
| Directory | `Examples/Apps/ClockfaceConsole` |
| `APP_NAME` / `APP_USER_NAME` | `Console` |
| `APP_ID` | `A1923FF0C81519B0` |
| `.uapp` size | ~79 KB |
| Design | Figma `1317:1104` (24-hour), `1317:1359` (12-hour) |
| Fonts | `IBMPlexMono-SemiBold.ttf` 85, `-Medium.ttf` 36/20/16, `Poppins-Medium.ttf` 14 |

## What it demonstrates

### A face trimmed to one sensor

It draws no charge level, no active minutes and no heart rate, so the service
subscribes to none of them and the messages, the model and the listener carry
only what is shown. `Commands.hpp` has `Time`, `Steps`, `ClockFormat` and
`Refresh`; the model holds a single `uint32_t` where the other faces hold a
triple; the simulator config disables the battery and heart-rate sensors to
match.

That is not tidiness. **A `HEART_RATE` subscription keeps the optical sensor
powered**, and a face is on screen for hours -- see the guide. This is the
example to copy when your design drops a row.

### A group centred on the digits, not on the whole thing

The clock group carries the meridiem, and the meridiem is deliberately **left
out of the width the group is centred on**:

```cpp
const int16_t total = hourWidth + sepWidth + minuteWidth;   // no meridiem
```

So the digits stay centred on the face and the `am` / `pm` hangs off to their
right. That is what the design does, and the reason is above the clock: the day
of month and the month are both centred, and the clock reads as the third line
of that stack only if its digits are too. Including the label would shift the
digits left and break the alignment. Measured against the design's 12-hour
frame, the digits centre on 121 against its 120.5.

`kSeparator12 = 22`, `kSeparator24 = 6`. IBM Plex Mono is monospaced -- every
glyph 600/1000 em, so 21.6 px at 36 -- which is why the 12-hour form needs no
extra padding around the colon the way Poppins does.

### Three stacked parts that need no layout at all

The weekday, day of month and month are each centred across the full width, so
writing them is the whole job. The weekday is spelled out in full (`WEDNESDAY`)
and the month abbreviated (`AUG`), which is why they use two typographies and
two sets of Text IDs -- both read back with `TypedText::getText()` into
wildcards, so both typographies need the letters in `WildcardCharacters`.

The meridiem is the exception: two static labels swapped with `setTypedText()`,
because it is the one part of the group whose text never varies beyond two
values.

## Assets

`StepsIcon_17x23.png` from the content pack, and two of IBM Plex Mono's
fourteen shipped weights -- only the two that are used are committed. Poppins is
here for the one row the design sets in it, the step count.

## Known gaps

- The steps row cannot be fed by the simulator (see the guide).
- The launcher icons are placeholders.
