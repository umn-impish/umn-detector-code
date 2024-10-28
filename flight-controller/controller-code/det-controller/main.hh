#pragma once
#include <Listener.hh>
#include <DetectorService.hh>
#include <DetectorSupport.hh>
#include <cstdlib>
#include <memory>

void usage(const char* prog);
int make_listen_socket();
std::unique_ptr<DetectorService> setup_service(int socket_fd);
std::unique_ptr<DetectorService> auto_health(std::unique_ptr<DetectorService> service);
