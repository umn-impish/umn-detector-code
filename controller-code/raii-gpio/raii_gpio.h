#ifndef RAII_GPIO_HEADER
#define RAII_GPIO_HEADER

#include <gpiod.h>
#include <unistd.h>
#include <stdexcept>
#include <cstdint>

/*
    A minimal RAII wrapper for the gpiod C API.
    Lets you read and write digital values to any GPIO pin.
*/

class GpioError : public std::runtime_error {
public:
    GpioError(const std::string& emsg);
};

class RaiiGpioPin {
public:
    enum class Operation {
        read,
        write,
        rising_edge,
        falling_edge
    };

    RaiiGpioPin(uint8_t broadcom_pin, Operation op, int default_value=0);
    ~RaiiGpioPin();

    bool read();
    void write(bool val);
    bool edge_event(const struct timespec& timeout);

    // std::move is allowed with C-style resources
    constexpr RaiiGpioPin(RaiiGpioPin&&) = default;
    // no copying (shared C-style resources)
    RaiiGpioPin(const RaiiGpioPin&) = delete;
    RaiiGpioPin& operator=(const RaiiGpioPin&) = delete;
private:
    int pin;
    struct gpiod_chip* chip;
    struct gpiod_line* line;
};

#endif
