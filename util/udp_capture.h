#ifndef UDP_CAPTURE_HEADER
#define UDP_CAPTURE_HEADER

#include <arpa/inet.h>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct ProgramArgs {
    int listen_port;
    std::optional<int> listen_timeout;
    std::optional<int> absolute_timeout;
    std::string base_fn;
    size_t max_fsz;
    std::string post_process;
    std::vector<sockaddr_in> forward_to;
};

struct Output {
    std::string name;
    std::ofstream file;

    void write(const char *dat, size_t size);
    void open(const std::string &base_fname);
    void close();
};

void init_traps();
void sig_handle(int sig);
void usage(const char *);
sockaddr_in extract_sockaddr_in(const std::string &addy);
void listen_write_loop(const ProgramArgs &args);
ProgramArgs parse_args(int argc, char *argv[]);
void post_process(const std::string &prog, const std::string &fn);
int initialize_socket(const ProgramArgs &args);

#endif
