#include <unistd.h>
#include <string>
#include <stdexcept>
#include "libcom.hpp"
#include "fmt/core.h"
#include "siteconf.hpp"

extern "C" {
    #include "keypress.h"
}

int main(int argc, char* argv[]) {
    try {
        const char *svc = argc > 1 ? argv[1] : "8000";
        server::tcp server1(svc, [](int peerfd){ 
            return network_channel::tls_channel::factory(peerfd, add_home_dir(getconf("server/cert", "calc.cert")), add_home_dir(getconf("server/key", "calc.key")));
        });
        fmt::print("Server is bound to port {}.\n", svc);

        server1.online(http::handler, (int) getconf("server/concurrency", 0.));
        puts("Server is online.");

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