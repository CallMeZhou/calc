#include <unistd.h>
#include <string>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include <signal.h>
#include "fmt/core.h"
#include "clipp.h"
#include "libcom.hpp"
#include "siteconf.hpp"

using namespace std;
using namespace filesystem;
using namespace clipp;
using namespace server;
using namespace server_utils;
using namespace network_channel;

static event main_thread_keeper;
void sigint_handler(int) {
    main_thread_keeper.signal();
}

int main(int argc, char* argv[]) {
    string http_port = "1080";
    string https_port = "1443";
    bool http_disabled = false;
    bool https_disabled = false;
    bool help = false;
    auto cli = (
        option("-p", "--port").doc(fmt::format("Override the default port for HTTP protocol. It binds to {} if this option is omitted.", http_port)) & value("http port", http_port),
        option("-s", "--ssl").doc(fmt::format("Override the default port for SSL/TLS protocol. It binds to {} if this option is omitted.", https_port)) & value("https port", https_port),
        option("--nohttp").set(http_disabled).doc(fmt::format("Disable HTTP connections. HTTP is enabled by default.")),
        option("--nossl").set(https_disabled).doc(fmt::format("Disable SSL/TLS connections. SSL/TLS is enabled by default.")),
        option("--home").doc(fmt::format("Override the home directory path. If this option is omitted, it gets the path from the environment variable ${}. (whose current value is '{}'.)", SITE_HOME_ENVAR, get_site_home())) & value("site home", get_site_home()),
        option("-h", "--help").set(help).doc("Show usge help.")
    );

    if (!parse(argc, argv, cli)) {
        cout << make_man_page(cli, path(argv[0]).filename()) << endl;
        return 1;
    }

    if (help) {
        cout << make_man_page(cli, path(argv[0]).filename()) << endl;
        return 0;
    }

    signal(SIGINT, sigint_handler);

    try {
        puts("CALC http server at your service.");

        thread_pool threads((int) getconf("server/concurrency", 0.));

        unique_ptr<tcp> http, https;
        unique_ptr<tls_contex> tls_ctx;

        if (not http_disabled) {
            puts("Starting HTTP server...");

            http.reset(new tcp(http_port, [](int peerfd){ return network_channel::tcp_channel::factory(peerfd); }));
            fmt::print("HTTP server is bound to port {}.\n", http_port);

            http->online(http::handler, threads);
            puts("HTTP server is online.");
        }

        if (not https_disabled) {
            puts("Loading certificate and private key...");
            tls_ctx.reset(new tls_contex(add_home_dir(getconf("server/cert", "calc.cert")), add_home_dir(getconf("server/key", "calc.key"))));

            puts("Starting HTTPS server...");

            https.reset(new tcp(https_port, [&tls_ctx](int peerfd){ return network_channel::tls_channel::factory(peerfd, *tls_ctx.get()); }));
            fmt::print("HTTPS server is bound to port {}.\n", https_port);

            https->online(http::handler, threads);
            puts("HTTPS server is online.");
        }

        puts("All servers are up online, press Ctrl+C or send SIGINT to stop...");
        main_thread_keeper.wait();
        puts("");
 
        if (http) {
            puts("Stopping HTTP server...");
            http->offline();
            puts("HTTP is offline.");
        }

        if (https) {
            puts("Stopping HTTPS server...");
            https->offline();
            puts("HTTPS is offline.");
        }

    } catch (std::exception &e) {
        fmt::print(stderr, "FAILED: {}.\n", e.what());
    }

    puts("All servers exited.");
    puts("CALC http server exited.");
    puts("Bye.");
    return 0;
}