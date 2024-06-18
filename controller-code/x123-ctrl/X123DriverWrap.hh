#pragma once

#include <memory>

#include "UsbConnectionManager.hh"
#include "packets/BasePacket.hh"
#include "packets/responses/Ack.hh"

class X123DriverWrap {
    size_t num_retries_;
    std::unique_ptr<X123Driver::UsbConnectionManager> cm;
    void guard_cm();
    void reinit();
public:
    X123DriverWrap();
    ~X123DriverWrap();

    X123DriverWrap(size_t n);

    size_t num_retries() const;
    void num_retries(size_t n);

    void send_recv(X123Driver::Packets::BasePacket& out, X123Driver::Packets::BasePacket& in);

    // move semantics == cleaner calls sometimes
    void send_recv(X123Driver::Packets::BasePacket&& out, X123Driver::Packets::BasePacket&& in);
    void send_recv(X123Driver::Packets::BasePacket&& out, X123Driver::Packets::BasePacket&  in);
    void send_recv(X123Driver::Packets::BasePacket&  out, X123Driver::Packets::BasePacket&& in);

    bool valid() const;
};
