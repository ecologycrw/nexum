# CA-06 of slice 1: the connection against a local test server, the order of the queues and the notices.
nexum_add_tests(nexum_net_tests
    SOURCES ConnectionTest.cpp
    DEPS Nexum::Net Nexum::NetTestServer)
