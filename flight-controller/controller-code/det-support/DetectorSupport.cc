#include <cstring>
#include <cstdlib>

#include <DetectorSupport.hh>

DetectorException::DetectorException(const std::string& what) :
    std::runtime_error(what)
{ }

ReconnectDetectors::ReconnectDetectors(const std::string& what) :
    std::runtime_error(what)
{ }

namespace Detector
{

constexpr auto SETTINGS_LOCATION = "/detector-config/";
SettingsSaver::SettingsSaver(std::string const& file_name)
  : file_path(std::getenv("HOME") + (SETTINGS_LOCATION + file_name))
{ }

SettingsSaver::~SettingsSaver()
{ }

std::string SettingsSaver::read() const {
    std::ifstream ifs{file_path, std::ios::binary};
    if (!ifs) {
        throw DetectorException("Can't open settings file " + file_path + " for reading");
    }

    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

void SettingsSaver::write(const std::string& dat) {
    std::ofstream ofs{file_path, std::ios::binary};
    if (!ofs) {
        throw DetectorException("Can't open settings file " + file_path + " for writing");
    }

    ofs << dat;
}

// Want a separate socket because the other one might be busy...
DataSaver::DataSaver(unsigned short udp_port) :
    sock_fd{socket(AF_INET, SOCK_DGRAM, 0)},
    destination{
        .sin_family = AF_INET,
        .sin_port = htons(udp_port),
        .sin_addr = {.s_addr = inet_addr("127.0.0.1")},
        .sin_zero = {0}
    }
{
	log_debug("udp port is: " + std::to_string(udp_port));
	if (sock_fd < 0) {
		throw DetectorException{"Can't bind sender socket: " + std::string{strerror(errno)}};
	}

	// need socket to block for data send
	int flags = fcntl(sock_fd, F_GETFL, 0);
	if (flags < 0)
		throw DetectorException{"Can't get sender socket flags"};

	flags &= ~O_NONBLOCK;
	if (fcntl(sock_fd, F_SETFL, flags) < 0)
		throw DetectorException{"Can't set flags for sender socket"};
}

DataSaver::~DataSaver()
{
	close(sock_fd);
}

void DataSaver::add(std::span<unsigned char const> data) {
    // we must not receive more than 64 KiB per transfer
    if (data.size() > std::numeric_limits<uint16_t>::max()) {
        throw DetectorException{
            "Cannot save data larger than 64 KiB. "
            "Dest. port is: " + std::to_string(ntohs(destination.sin_port))
        };
    }

    std::vector<char> to_send;
    auto data_size = static_cast<uint16_t>(data.size());
    to_send.resize(data_size);
    std::memcpy(to_send.data(), data.data(), data_size);

    constexpr socklen_t SOCKADDR_SZ = sizeof(sockaddr_in);
    int ret = sendto(
        sock_fd,
        to_send.data(),
        to_send.size(),
        0,
        (const sockaddr*)&destination,
        SOCKADDR_SZ
    );

    if (ret < 0) {
        throw DetectorException{std::string{"Sendto error: "} + strerror(errno)};
    }
}
void DataSaver::add(std::span<char const> data) {
  // Call unsigned char overload
  add({reinterpret_cast<unsigned char const*>(data.data()), data.size()});
}

QueuedDataSaver::QueuedDataSaver(
    unsigned short udp_port,
    size_t nbs
) :
    ds{std::make_unique<DataSaver>(udp_port)},
    data_blob{},
    num_before_save{nbs}
{ }

QueuedDataSaver::~QueuedDataSaver()
{ }

bool QueuedDataSaver::add(QueuedDataSaver::msg_t const& m) {
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

void QueuedDataSaver::save_blob() {
    auto num_bytes = num_before_save * sizeof(msg_t);
    ds->add({
      reinterpret_cast<unsigned char*>(data_blob.data()),
      num_bytes
    });

    // if we have added another timestamped piece of data,
    // this will put it at the front
    // if there are only `num_before_save` pieces of data,
    // the iterators will be equal and nothign will be copied (begin + nbs == end)
    data_blob = std::vector<msg_t>(data_blob.begin() + num_before_save, data_blob.end());
}

X123Tables::X123Tables(int base_port) : 
    science{std::make_shared<DataSaver>(base_port)},
    debug{std::make_shared<DataSaver>(base_port + 1)}
{ }

static constexpr size_t OFFSET = 3;
static constexpr size_t NUM_SLICES_PER_SEC = 32;
HafxTables::HafxTables(int chan, int base_port) :
    time_slice{std::make_shared<QueuedDataSaver>(base_port + 0 + chan*OFFSET, NUM_SLICES_PER_SEC)},
    list{std::make_shared<DataSaver>(            base_port + 1 + chan*OFFSET)},
    debug{std::make_shared<DataSaver>(           base_port + 2 + chan*OFFSET)}
{ }
}
