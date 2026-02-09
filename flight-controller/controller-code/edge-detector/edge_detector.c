#include <errno.h>
#include <gpiod.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void log_error(const char* s) {
    fprintf(stderr, "%s: %s\n", s, strerror(errno));
}

/*
 * Detect a rising edge event on gpiochip0 for a given GPIO pin
 *
 * Returns: 0 on success, 1 on timeout, 2 on other failure.
 * */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        errno = EINVAL;
        log_error("Provide GPIO pin for edge detection as only argument");
        return 2;
    }
    // Defaults to GPIO 0 for invalid input
    const unsigned int detect_pin = atoi(argv[1]);

    // Initialize these here for C-style cleanup
    struct gpiod_chip *chip = nullptr;
    struct gpiod_line_config *cfg = nullptr;
    struct gpiod_line_settings * settings = nullptr;
    struct gpiod_line_request *line_req = nullptr;
    int edge_status = -1;
    // nanosecond edge event timeout
    static constexpr int64_t timeout = 2'000'000'000;

    int exit_code = 0;

    chip = gpiod_chip_open("/dev/gpiochip0");
    if (chip == nullptr) {
        log_error("Can't open GPIO chip for PPS detect");
        exit_code = 2;
        goto quit;
    }

    cfg = gpiod_line_config_new();
    if (cfg == nullptr) {
        log_error("Can't create line config");
        exit_code = 2;
        goto quit;
    }

    settings = gpiod_line_settings_new();
    if (settings == nullptr) {
        log_error("Can't create gpio line settings");
        exit_code = 2;
        goto quit;
    }

    if (gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT) != 0) {
        log_error("Cannot set direction of GPIOD line settings to input");
        exit_code = 2;
        goto quit;
    }

    if (gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_RISING) != 0) {
        log_error("Cannot set rising edge mode setting");
        exit_code = 2;
        goto quit;
    }

    if (gpiod_line_config_add_line_settings(cfg, &detect_pin, 1, settings) != 0) {
        log_error("Cannot set GPIOD line config settings for PPS pin");
        exit_code = 2;
        goto quit;
    }

    line_req = gpiod_chip_request_lines(
        chip, nullptr, cfg
    );
    if (line_req == nullptr) {
        log_error("Can't get GPIO line request for PPS pin");
        exit_code = 2;
        goto quit;
    }

    edge_status = gpiod_line_request_wait_edge_events(line_req, timeout);
    if (edge_status == 1) {
        exit_code = 0;
    } else if (edge_status == 0) {
        exit_code = 1;
    } else {
        log_error("There was an issue watching for edge events");
        exit_code = 2;
    }

quit:
    gpiod_line_settings_free(settings);
    gpiod_line_request_release(line_req);
    gpiod_line_config_free(cfg);
    gpiod_chip_close(chip);
    return exit_code;
}
