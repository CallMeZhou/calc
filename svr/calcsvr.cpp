#include <string>
#include <stdexcept>
#include "libcom.hpp"
#include "fmt/core.h"

extern "C" {
    #include "keypress.h"
}

int main(int argc, char* argv[]) {
    try {
        const char *svc = argc > 1 ? argv[1] : "8000";
        server::tcp server1(svc);
        printf("Server is bound to port %s.\n", svc);

        server1.online(http::handler);
        printf("Server is online.\n");

        puts("Press any key to stop...");
        keypress(KP_ECHO_OFF);

        server1.offline();
        puts("Server is offline.");

    } catch (std::exception &e) {
        fmt::print(stderr, "FAILED: {}.\n", e.what());
    }

    printf("Server exited.\n");
    return 0;
}