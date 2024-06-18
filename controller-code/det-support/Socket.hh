#pragma once
#include <csignal>
#include <cstring>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <stdexcept>

class Socket {
    int fd_;

public:
    int fd() const { return fd_; }

    Socket() =delete;
    explicit Socket(int me_port) :
        fd_{socket(AF_INET, SOCK_DGRAM, 0)}
    {
        if (fd_ < 0) {
            throw std::runtime_error{
                "cannot open socket for listening: " + std::string{strerror(errno)}};
        }

        sockaddr_in listen {
            .sin_family = AF_INET,
            .sin_port = htons(me_port),
            .sin_addr = {.s_addr = INADDR_ANY},
            .sin_zero = {0}
        };
        if (bind(fd_, (sockaddr*)&listen, sizeof(listen)) < 0)
            throw std::runtime_error{
                "cannot bind socket to listen port: " + std::string{strerror(errno)}};
    }

    ~Socket() {
        close(fd_);
    }
};
