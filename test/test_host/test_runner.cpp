#include "test_host.h"

int main()
{
    struct TestEntry {
        const char *name;
        bool (*fn)();
    };

    TestEntry tests[] = {
        {"build", run_test_build},
        {"url_decode", run_test_url_decode},
        {"parser", run_test_parser},
        {"request", run_test_request},
        {"response", run_test_response},
        {"router", run_test_router},
        {"static_path", run_test_static_path},
        {"mime", run_test_mime},
        {"engine", run_test_engine},
    };

    for (unsigned i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        if (!tests[i].fn()) {
            printf("FAILED %s\n", tests[i].name);
            return 1;
        }
        printf("OK %s\n", tests[i].name);
    }
    return 0;
}
