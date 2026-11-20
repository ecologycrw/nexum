// The main function of every test program (docs/adr/0025-ferramentas-de-verificacao.md).
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <cstdlib>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

namespace {

// On Windows, a failed assertion or an abort opens a dialog that stops the test until someone clicks it.
// The failure is written as text instead, so it shows in the output of the verification and of the CI.
void ReportFailuresAsText() noexcept {
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#if defined(_DEBUG)
    for (const int kind : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT}) {
        _CrtSetReportMode(kind, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(kind, _CRTDBG_FILE_STDERR);
    }
#endif
#endif
}

} // namespace

int main(int argc, char** argv) {
    ReportFailuresAsText();
    doctest::Context context(argc, argv);
    return context.run();
}
