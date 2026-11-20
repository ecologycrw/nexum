#include <doctest/doctest.h>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>

namespace {

// The mode can combine several outputs. What matters is: written as text, and never a dialog.
bool WritesTextWithoutDialog(int reportType) {
    const int mode = _CrtSetReportMode(reportType, _CRTDBG_REPORT_MODE);
    return (mode & _CRTDBG_MODE_FILE) != 0 && (mode & _CRTDBG_MODE_WNDW) == 0;
}

} // namespace
#endif

TEST_CASE("a failed assertion is written as text, without opening a dialog") {
#if defined(_MSC_VER) && defined(_DEBUG)
    CHECK(WritesTextWithoutDialog(_CRT_ASSERT));
    CHECK(WritesTextWithoutDialog(_CRT_ERROR));
#else
    // Only the debug runtime of MSVC opens these dialogs.
    CHECK(true);
#endif
}
