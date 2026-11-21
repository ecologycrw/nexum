#include <doctest/doctest.h>

#include "Nexum/Platform/FailureReports.hpp"

#if defined(_MSC_VER)
#include <cstdlib>
#endif
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

using nexum::platform::AsksForFailuresAsText;

TEST_CASE("only a value other than empty or 0 asks for the failures as text") {
    CHECK_FALSE(AsksForFailuresAsText(nullptr));
    CHECK_FALSE(AsksForFailuresAsText(""));
    CHECK_FALSE(AsksForFailuresAsText("0"));
    CHECK(AsksForFailuresAsText("1"));
    CHECK(AsksForFailuresAsText("yes"));
}

// Only the C runtime of Windows opens these dialogs; elsewhere the call changes nothing, and there is nothing
// to check.
#if defined(_MSC_VER)
TEST_CASE("after WriteFailuresAsText, an abort and a failed assertion are text, without a dialog") {
    // The main function of the tests already writes the failures as text, so the dialogs come back first.
    _set_abort_behavior(_WRITE_ABORT_MSG | _CALL_REPORTFAULT, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#if defined(_DEBUG)
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
#endif

    nexum::platform::WriteFailuresAsText();

    // A mask of 0 reads the flags without changing them.
    const unsigned int abortFlags = _set_abort_behavior(0, 0);
    CHECK((abortFlags & _WRITE_ABORT_MSG) == 0);
    CHECK((abortFlags & _CALL_REPORTFAULT) == 0);
#if defined(_DEBUG)
    for (const int kind : {_CRT_ASSERT, _CRT_ERROR}) {
        const int mode = _CrtSetReportMode(kind, _CRTDBG_REPORT_MODE);
        CHECK((mode & _CRTDBG_MODE_FILE) != 0);
        CHECK((mode & _CRTDBG_MODE_WNDW) == 0);
    }
#endif
}
#endif
