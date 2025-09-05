#include "SDK/Wrappers/StdLibWrappers.h"

#include <string.h>

// Wrapper gmtime
struct tm safe_gmtime(const time_t* time)
{
    struct tm result;

#ifdef _WIN32
    gmtime_s(&result, time);    // Windows
#else
    gmtime_r(time, &result);    // Linux, POSIX
#endif

    return result;
}

// Wrapper gmtime
struct tm safe_localtime(const time_t* time)
{
    struct tm result;

#ifdef _WIN32
    localtime_s(&result, time);   // Windows
#else
    localtime_r(time, &result);   // Linux, POSIX
#endif

    return result;
}

// Wrapper strncpy
void safe_strcpy(char* dest, const char* src, size_t destSize)
{
    if (destSize == 0) {
        return;
    }

#ifdef _WIN32
    strncpy_s(dest, destSize, src, _TRUNCATE);      // Windows
#else
    std::strncpy(dest, src, destSize - 1);          // Linux, POSIX
    dest[destSize - 1] = '\0';
#endif
}
