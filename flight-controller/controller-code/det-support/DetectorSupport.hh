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
template<class MessageT>
class QueuedDataSaver {
public:
    using msg_t = MessageT;
    QueuedDataSaver(unsigned short udp_port, size_t num_before_save) :
        ds{std::make_unique<DataSaver>(udp_port)},
        data_blob{},
        num_before_save{num_before_save}
    { }

    ~QueuedDataSaver()
    { }

    // returns false if data is not added
    bool add(msg_t const& m) {
        bool skip_data_piece = (data_blob.empty() && m.time_anchor < 1);
        if (skip_data_piece) {
            return false;
        }

        data_blob.push_back(m);

        if (data_blob.size() >= num_before_save) {
            save_blob();
        }

        return true;
    }

private:
    std::unique_ptr<DataSaver> ds;
    std::vector<msg_t> data_blob;
    size_t num_before_save;

    void save_blob() {
        auto num_bytes = num_before_save * sizeof(msg_t);
        ds->add({
          reinterpret_cast<unsigned char*>(data_blob.data()),
          num_bytes
        });

        // if we have added another timestamped piece of data,
        // this will put it at the front
        // if there are only `num_before_save` pieces of data,
        // the iterators will be equal and nothing will be copied (begin + nbs == end)
        data_blob = std::vector<msg_t>(data_blob.begin() + num_before_save, data_blob.end());
    }
};

struct DetectorPorts {
    unsigned short science;
    unsigned short debug;
};

} // namespace Detector

