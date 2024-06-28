#include "raii_gpio.h"
#include <cstring>

GpioError::GpioError(const std::string& emsg) :
    std::runtime_error(emsg)
{ }


RaiiGpioPin::RaiiGpioPin(uint8_t broadcom_pin, RaiiGpioPin::Operation op, int default_value) :
    pin(broadcom_pin),
    chip(nullptr),
    line(nullptr)
{
    static const char* CHIP_NAME = "gpiochip0";
    chip = gpiod_chip_open_by_name(CHIP_NAME);
    if (!chip) {
        std::string s{"cannot open gpio chip " + std::string(CHIP_NAME)};
        throw GpioError{s};
    }

    line = gpiod_chip_get_line(chip, pin);
    if (!line) {
        if (chip) {
            gpiod_chip_close(chip);
            chip = nullptr;
        }
        std::string s{"cannot open GPIO pin " + std::to_string(pin)};
        throw GpioError{s};
    }
    
    static const char* CONSUMER = "gpio-user";
    std::string error_str{};
    switch (op) {
        case RaiiGpioPin::Operation::read:
            if (gpiod_line_request_input(line, CONSUMER) < 0) {
                error_str = "cannot request pin " + std::to_string(pin) + " as input";
            }
            break;
        case RaiiGpioPin::Operation::write:
            if (gpiod_line_request_output(line, CONSUMER, default_value) < 0) {
                error_str = "cannot request pin " + std::to_string(pin) + " as output";
            }
            break;
        case RaiiGpioPin::Operation::rising_edge:
            if (gpiod_line_request_rising_edge_events(line, CONSUMER) < 0) {
                error_str = "cannot request pin " + std::to_string(pin) + " as rising edge listen";
            }
            break;
        case RaiiGpioPin::Operation::falling_edge:
            if (gpiod_line_request_falling_edge_events(line, CONSUMER) < 0) {
                error_str = "cannot request pin " + std::to_string(pin) + " as falling edge listen";
            }
            break;
    }

    if (!error_str.empty()) {
        if (line) gpiod_line_release(line);
        if (chip) gpiod_chip_close(chip);
        throw GpioError{error_str};
    }
}

RaiiGpioPin::~RaiiGpioPin() {
    if (line) {
        gpiod_line_release(line);
        line = nullptr;
    }
    if (chip) {
        gpiod_chip_close(chip);
        chip = nullptr;
    }
}

bool RaiiGpioPin::read() {
    int ret = gpiod_line_get_value(line);
    if (ret < 0) {
        throw GpioError{
            "failed to read from pin " + std::to_string(pin) + ": "
            + std::string{strerror(errno)}
        };
    }

    return static_cast<bool>(ret);
}

void RaiiGpioPin::write(bool val) {
    int ret = gpiod_line_set_value(line, static_cast<int>(val));
    if (ret < 0) {
        throw GpioError{
            "failed to write to pin " + std::to_string(pin) + ": "
            + std::string{strerror(errno)}
        };
    }
}

bool RaiiGpioPin::edge_event(const struct timespec& timeout) {
    int ret = gpiod_line_event_wait(line, &timeout);
    if (ret < 0) {
        throw GpioError{
            "error reading edge event on pin " + std::to_string(pin) + ": "
            + std::string{strerror(errno)}
        };
    }

    // true if event, false if timeout
    return (ret == 1);
}
