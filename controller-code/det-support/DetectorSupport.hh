#pragma once

#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <stdexcept>
#include <memory>
#include <filesystem>
#include <fstream>
#include <map>
#include <queue>
#include <span>
#include <string>
#include <sstream>
#include <unordered_map>

#include <logging.hh>
#include <DetectorMessages.hh>

class DetectorException : public std::runtime_error {
public:
    DetectorException(const std::string& what);
};

class ReconnectDetectors : public std::runtime_error {
public:
    ReconnectDetectors(const std::string& what);
};

namespace Detector {

class SettingsSaver {
    const std::string file_path;
public:
    SettingsSaver(std::string const& file_name);
    ~SettingsSaver();

    std::string read() const;
    void write(std::string const& dat);

    template<typename T>
    T read_struct() const {
        auto str = read();
        if (str.size() != sizeof(T)) {
            throw DetectorException("File size did not match struct size for settings file " + file_path);
        }

        T t;
        memcpy(&t, str.data(), str.size());
        return t;
    }

    template<typename T>
    void write_struct(T const& t) {
        auto str = std::string {};
        str.resize(sizeof(t));
        memcpy(str.data(), &t, sizeof(t));

        write(str);
    }
};

class DataSaver {
    int sock_fd;
    const sockaddr_in destination;

public:
    DataSaver() =delete;
    DataSaver(unsigned short udp_port);

    ~DataSaver();

    void add(std::span<unsigned char const> data);
    void add(std::span<char const> data);
};

/*
 * For nominal science data, we want things to be well-aligned with files.
 * So this lets us save a fixed number of events per file and ensure that each
 * file starts with a data chunk that has a valid time.
 * */
class QueuedDataSaver {
public:
    using msg_t = DetectorMessages::HafxNominalSpectrumStatus;

    QueuedDataSaver(unsigned short udp_port, size_t num_before_save);
    ~QueuedDataSaver();

    // returns false if data is not added
    bool add(msg_t const& m);

private:
    std::unique_ptr<DataSaver> ds;
    std::vector<msg_t> data_blob;
    size_t num_before_save;

    void save_blob();
};

struct BasePorts {
    unsigned short x123;
    unsigned short hafx;
};

struct X123Tables {
    std::shared_ptr<DataSaver> science;
    std::shared_ptr<DataSaver> debug;

    X123Tables(int base_port);
};

struct HafxTables {
    std::shared_ptr<QueuedDataSaver> time_slice;
    std::shared_ptr<DataSaver> list;
    std::shared_ptr<DataSaver> debug;

    HafxTables() =delete;
    HafxTables(int chan, int base_port);
};

class Settings {
    using HafxChannel = DetectorMessages::HafxChannel;
public:
    DetectorMessages::X123Settings x123;
    std::unordered_map<HafxChannel, DetectorMessages::HafxSettings> hafx;
    Settings();
};

}

