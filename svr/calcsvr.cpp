#include <stdio.h>
#include <string>
#include <stdexcept>
#include <algorithm>
#include "libcom.hpp"
#include "ascasvr.hpp"

extern "C" {
    #include "keypress.h"
}

struct ServiceEcho : SimpleHandler {
    ServiceEcho(int peer) : SimpleHandler(sizeof(BaseHeader), peer) {}

    ReturnCode handleRequest(const std::vector<uint8_t> &request, BaseHeader *header, std::vector<uint8_t> &response) const override {
        constexpr char PREFIX[] = "ECHO: ";
        std::string reqstr((char*) &request[0], request.size());
        response.resize(reqstr.length() + sizeof(PREFIX) + 1);
        sprintf((char*) &response[0], "%s%s", PREFIX, reqstr.c_str());
        return ReturnCode::Succeeded;
    }

    ~ServiceEcho() {
        printf("ServiceEcho-%p done.\n", this);
    }

    static StreamServerHandler* factory(int peer) {
        return new ServiceEcho(peer);
    }
};

int main(int argc, char* argv[]) {
    try {
        const char *svc = argc > 1 ? argv[1] : "8000";
//        StreamServer server1(svc, ServiceEcho::factory);
        StreamServer server1(svc, ServiceAsciiart::factory);
        printf("Server is online at port %s.\n", svc);
        server1.online();
        puts("Press any key to stop...");
        keypress(KP_ECHO_OFF);
        server1.offline();
        puts("Server is offline.");
    } catch (std::exception &e) {
        fprintf(stderr, "FAILED: %s.\n", e.what());
    }

    printf("Server exited.\n");
    return 0;
}