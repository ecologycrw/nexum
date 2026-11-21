# CA-08 of specs/0000-fundacao/spec.md: the comparison of visualproof, and its command line.
nexum_add_tests(visualproof_tests
    SOURCES
        ImageComparisonTest.cpp
        ProofOptionsTest.cpp
    DEPS visualproof_comparison Nexum::Platform)
