#include "stdio.h"
#include "libcom.hpp"
#include <stdexcept>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        puts("Usage: echosvc <message to echo> [remote host name or ip] [port]");
        return 1;
    }

    try {
        puts(echo(
            argc >= 3 ? argv[2] : "locahost", 
            argc >= 4 ? argv[3] : "8000", 
            argv[1]).c_str());
    } catch (std::exception &e) {
        fprintf(stderr, "FAILED: %s.\n", e.what());
        return 2;
    }

    return 0;
}