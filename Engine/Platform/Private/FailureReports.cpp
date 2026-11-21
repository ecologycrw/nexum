#include "Nexum/Platform/FailureReports.hpp"

#include <string_view>

#include <SDL3/SDL.h>

#if defined(_MSC_VER)
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#endif
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif
#if defined(_WIN32)
// WIN32_LEAN_AND_MEAN and NOMINMAX come from Engine/Platform/CMakeLists.txt.
#include <windows.h>
#endif

namespace nexum::platform {

namespace {

#if defined(_MSC_VER)
// A function of the C runtime called with a bad argument, such as a null string: without this, the runtime
// ends the program through the error reporting of Windows, with no message of its own. In the debug build,
// the failed assertion that comes before it is already written.
void EndOnInvalidParameter(const wchar_t* /*expression*/, const wchar_t* /*function*/,
                           const wchar_t* /*file*/, unsigned int /*line*/, std::uintptr_t /*reserved*/) {
    std::fputs("a function of the C runtime received an invalid parameter\n", stderr);
    std::abort();
}
#endif

} // namespace

bool AsksForFailuresAsText(const char* value) noexcept {
    if (value == nullptr) {
        return false;
    }
    const std::string_view text(value);
    return !text.empty() && text != "0";
}

void WriteFailuresAsText() noexcept {
#if defined(_MSC_VER)
    // abort() would show its message in a dialog and then ask Windows to report the failure, with another
    // one.
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _set_invalid_parameter_handler(EndOnInvalidParameter);
#if defined(_DEBUG)
    // Only the debug runtime has these reports: the failed assertions of the standard library among them.
    for (const int kind : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT}) {
        _CrtSetReportMode(kind, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(kind, _CRTDBG_FILE_STDERR);
    }
#endif
#endif
#if defined(_WIN32)
    // A crash, such as a read of invalid memory or a failed check of the standard library, ends the program
    // without the "stopped working" dialog and without the slow error reporting of Windows. Measured with an
    // index outside a vector in the MuClient: 0.5 s with this, 4 to 5 s without it, and 28 s the first time.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
}

void WriteFailuresAsTextWhenAsked() noexcept {
    if (AsksForFailuresAsText(SDL_getenv(kFailuresAsTextVariable))) {
        WriteFailuresAsText();
    }
}

} // namespace nexum::platform
