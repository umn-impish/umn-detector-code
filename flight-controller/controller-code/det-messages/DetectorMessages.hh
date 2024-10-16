#pragma once

#include <netinet/in.h>
#include <IoContainer.hh>

#include <array>
#include <ctime>
#include <future>
#include <optional>
#include <variant>
#include <vector>
#include <type_traits>

namespace DetectorMessages {
    enum class HafxChannel : uint8_t {
        C1,
        M1,
        M5,
        X1,
    };
    struct HafxSettings {
        HafxChannel ch;

        uint16_t adc_rebin_edges_length;
        std::array<uint16_t, 2048> adc_rebin_edges;

        bool fpga_ctrl_regs_present;
        std::array<uint16_t, 16> fpga_ctrl_regs;

        bool arm_ctrl_regs_present;
        std::array<float, 64> arm_ctrl_regs;

        bool arm_cal_regs_present;
        std::array<float, 64> arm_cal_regs;

        bool fpga_weights_regs_present;
        std::array<uint16_t, 16> fpga_weights_regs;
    };

    struct HafxDebug {
        enum class Type : uint8_t {
            ArmCtrl,
            ArmCal,
            ArmStatus,
            FpgaCtrl,
            FpgaOscilloscopeTrace,
            FpgaStatistics,
            FpgaWeights,
            Histogram,
            ListMode,
        };
        Type type;
        uint32_t wait_between;
        HafxChannel ch;
    };

    struct X123Settings {
        explicit X123Settings();

        bool ack_err_retries_present;
        uint32_t ack_err_retries;

        uint16_t ascii_settings_length;
        std::array<char, 512> ascii_settings;

        uint16_t adc_rebin_edges_length;
        std::array<uint32_t, 128> adc_rebin_edges;
    };

    struct X123Debug {
        enum class Type : uint8_t {
            Histogram,
            Diagnostic,
            AsciiSettings,
        };
        Type type;
        std::string ascii_settings_query;
        uint32_t histogram_wait;
    };

    struct Shutdown { };

    // persistent setting:
    //   - which detectors should we use?
    struct Initialize { };
    struct CollectNominal { bool started = false; };
    struct StopNominal { };

    struct QueryTraceAcquisition { DetectorMessages::HafxChannel ch; };
    struct QueryLegacyHistogram { DetectorMessages::HafxChannel ch; };
    struct QueryListMode { DetectorMessages::HafxChannel ch; };
    struct QueryX123DebugHistogram { };

    struct ImmediateX123BufferRead { };
    struct RestartX123SequentialBuffering { };

    struct RestartTimeSliceCollection { };
    struct ImmediateHafxTimeSliceRead { };

    struct StartNrlList { bool started = false; };
    struct Start_Debug_NrlList { bool started = false; };
    struct StopNrlList { };

    struct __attribute__((packed)) HafxHealth {
        // 0.01K / tick
        uint16_t arm_temp;
        // 0.01K / tick
        uint16_t sipm_temp;
        // 0.01V / tick
        uint16_t sipm_operating_voltage;
        uint16_t sipm_target_voltage;

        uint32_t counts;
        // clock cycles
        uint32_t dead_time;
        // clock cycles
        uint32_t real_time;
    };
    struct __attribute__((packed)) X123Health {
        // 1 degC / tick
        int8_t board_temp;
        // 0.5V / tick
        int16_t det_high_voltage;
        // 0.1K / tick
        uint16_t det_temp;
        // 1 count / tick
        uint32_t fast_counts;
        uint32_t slow_counts;

        // 1ms / tick
        uint32_t accumulation_time;
        // 1ms / tick
        uint32_t real_time;
    };
    struct __attribute__((packed)) HealthPacket {
        uint32_t timestamp;
        HafxHealth c1;
        HafxHealth m1;
        HafxHealth m5;
        HafxHealth x1;
        X123Health x123;
    };

    struct ManualHealthPacket {
        struct sockaddr_in destination;
    };

    struct StartPeriodicHealth {
        uint32_t seconds_between;
        std::vector<sockaddr_in> fwd;
    };

    struct StopPeriodicHealth { };

    using DetectorCommand = std::variant<
        Initialize,
        Shutdown,
        CollectNominal,
        StopNominal,
        StartNrlList,
        Start_Debug_NrlList,
        StopNrlList,
        X123Settings,
        HafxSettings,
        HafxDebug,
        X123Debug,
        StartPeriodicHealth,
        StopPeriodicHealth
    >;
    struct PromiseWrap {
        std::promise<std::string> prom;
        DetectorCommand wrap_msg;
    };

    // Sep 2023: make this fixed-size so we can be precise with timing
    // Oct 2023: fix histogram size. d'oh
    struct __attribute__((packed)) HafxNominalSpectrumStatus {
        uint8_t ch;
        uint16_t buffer_number;
        uint32_t num_evts;
        uint32_t num_triggers;
        uint32_t dead_time;
        uint32_t anode_current;
        uint32_t histogram[123];

        uint32_t time_anchor;
        bool missed_pps;
    };

    // Strip down the NRL data to only the bits we care about
    struct __attribute__((packed)) StrippedNrlDataPoint {
        // Assumes that the 51-bit wall clock never exceeds
        // ~5-10 seconds, which should be reasonable in this
        // application.
        // Assumes we scale down to 200ns precision (8 25ns divs),
        // so with 25 bits, that gives a max time of 6.7s.
        uint32_t wall_clock   : 25;
        uint32_t energy       : 4;
        uint32_t was_pps      : 1;
        uint32_t piled_up     : 1;
        uint32_t out_of_range : 1;

        static StrippedNrlDataPoint
        from(const SipmUsb::NrlListDataPoint& orig);
    };
    static_assert(
        std::is_trivially_copyable_v<StrippedNrlDataPoint>,
        "Stripped NRL struct must be a 'POD' type"
    );
    // Size of the _data_ is a uint32, even with the static method
    static_assert(sizeof(StrippedNrlDataPoint) == sizeof(uint32_t));
}
