#include <X123DriverWrap.hh>
#include <DetectorSupport.hh>
#include <memory>

void X123DriverWrap::guard_cm() {
    if (cm == nullptr)
        throw DetectorException("X123 driver is null");
}

X123DriverWrap::X123DriverWrap() :
    X123DriverWrap(1)
{ }

X123DriverWrap::X123DriverWrap(size_t n) :
    num_retries_{n > 0? n : 1},
    cm{nullptr}
{
    reinit();
}

X123DriverWrap::~X123DriverWrap() { }

void X123DriverWrap::reinit() {
    try {
        cm = std::make_unique<X123Driver::UsbConnectionManager>();
    }
    catch (LibUsbCpp::UsbException const& e) {
        log_warning("USB issue: " + std::string{e.what()});
    }
}

size_t X123DriverWrap::num_retries() const {
    return num_retries_;
}

void X123DriverWrap::num_retries(size_t n) {
    num_retries_ = (n > 0)? n : 1;
}

void X123DriverWrap::send_recv(
    X123Driver::Packets::BasePacket& out,
    X123Driver::Packets::BasePacket& in
) {
    size_t i = 0;
    std::string error_message;
    do {
        guard_cm();
        try {
            cm->sendAndReceive(out, in);
            return;
        } 
        catch (X123Driver::Packets::AckError const& e) {
            error_message = 
                X123Driver::Packets::Responses::Ack{e}.issue();
            log_debug("ack error: " + error_message);
            ++i;
        }
        catch (const LibUsbCpp::UsbException& e) {
            throw ReconnectDetectors{"X123 USB issue:" + std::string{e.what()}};
        }
    } while (i < num_retries_);
    throw DetectorException{"x123 timed out or failed too many times: " + error_message};
}

// move semantics == cleaner calls sometimes
void X123DriverWrap::send_recv(
    X123Driver::Packets::BasePacket&& out,
    X123Driver::Packets::BasePacket&& in
) {
    auto& out_ref = out;
    auto& in_ref = in;
    send_recv(out_ref, in_ref);
}

void X123DriverWrap::send_recv(
    X123Driver::Packets::BasePacket& out,
    X123Driver::Packets::BasePacket&& in
) {
    auto& in_ref = in;
    send_recv(out, in_ref);
}

void X123DriverWrap::send_recv(
    X123Driver::Packets::BasePacket&& out,
    X123Driver::Packets::BasePacket& in
) {
    auto& out_ref = out;
    send_recv(out_ref, in);
}

bool X123DriverWrap::valid() const {
    return cm != nullptr;
}
