#pragma once

#include <string>
#include <vector>
#include <set>
#include <cstring>
#include <libusb-1.0/libusb.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>


namespace PBKR
{
static constexpr uint16_t DMXIS_VENDOR_ID{0x0403};
static constexpr uint16_t DMXIS_PRODUCT_ID{0x6001};
static constexpr uint8_t DEFAULT_DMX_MAX_VAL{127}; // Technically 255, but visual max is about 100
static constexpr uint16_t DEFAULT_DMX_MIN_VAL{1}; // 0 is to avoid. 1 is nicer
static constexpr uint16_t DEFAULT_DMX_POLL_MS{30}; // 40 = value observed on DMXIS sw

class DmxIs
{
public:
    using ByteVect=std::vector<uint8_t>;
    using IndexSet=std::set<uint16_t>;
    DmxIs(uint16_t vendor_id = DMXIS_VENDOR_ID, uint16_t product_id = DMXIS_PRODUCT_ID);
    ~DmxIs();

    void join();
    void setLed(uint8_t idx, uint8_t val);

    uint8_t* getLeds512();

    inline std::string getSN()const {return m_sn;};
    void setPowerRange(const uint8_t pMin, const uint8_t pMax);
    void breathTest(const IndexSet& lines={});
    static inline  DmxIs* instance(){return m_instance;}
    inline uint8_t getMin() const {return m_pMin;}
    inline uint8_t getMax() const {return m_pMax;}

private:

    bool start(int64_t sendPeriodMs = DEFAULT_DMX_POLL_MS);
    void tryReloadDevice();
    void resetDevice();
    uint16_t readReg90();
    void bulkWrite(const ByteVect& data, unsigned timeout = 1000);
    int bulkRead(uint8_t* buffer, unsigned maxSize, unsigned timeout = 1000);

    void threadImpl(int64_t sendPeriodMs);
    void updateBreathValue(void);

    const uint16_t m_vendor_id;
    const uint16_t m_product_id;
    std::string m_sn;
    static DmxIs* m_instance;
    uint8_t m_pMin{DEFAULT_DMX_MIN_VAL};
    uint8_t m_pMax{DEFAULT_DMX_MAX_VAL};
    uint16_t m_idx{0};
    libusb_context *m_ctx{nullptr};
    libusb_device_handle* m_devh{nullptr};
    ByteVect m_ledsTx;
    IndexSet m_breathTest;
    uint8_t m_breathValue{0};

    mutable std::mutex m_mutex;
    using Lock = std::lock_guard<std::mutex>;
    std::atomic_bool m_running{false};
    std::thread m_thread;
    std::chrono::steady_clock::time_point m_retryCnx;

    unsigned m_nbFailures{0};

}; // class DmxIs

}  // namespace PBKR
