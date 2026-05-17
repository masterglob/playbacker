#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <cmath>

#include "pbkr_dmx.h"

#if 0 // 1 = debug/test
#define PRINT printf
#define BREATH_TEST {8,10}
#else
#define PRINT(...)
#define BREATH_TEST {}
#endif


#define NB_LEDS 512

#define BUFF90_NB_HDR 12u

namespace
{
class UsbDeviceLostException : public std::runtime_error
{
public:
    UsbDeviceLostException(const std::string& msg)
        : std::runtime_error(msg) {}
};
}

namespace PBKR
{
using namespace std;

DmxIs* DmxIs::m_instance{nullptr};

/***********************************************************************/DmxIs::DmxIs(uint16_t vendor_id, uint16_t product_id):
                m_vendor_id(vendor_id),
                m_product_id(product_id),
                m_ledsTx(NB_LEDS)
{
    m_sn = "N/C";
    m_instance = this;
    libusb_init(&m_ctx);
    start();
    breathTest(BREATH_TEST);
}

/***********************************************************************/DmxIs::~DmxIs(){
    PRINT("~DmxIs-In\n");
    join();
    PRINT("~DmxIs-Mid1\n");
    resetDevice();
    PRINT("~DmxIs-Mid2\n");
    libusb_exit(m_ctx);
    PRINT("~DmxIs-Out\n");
    m_instance = nullptr;
}

/***********************************************************************/
bool DmxIs::start(int64_t sendPeriodMs)
{
    if (m_thread.joinable())
        return false;
    m_thread = std::thread([this, sendPeriodMs](){threadImpl(sendPeriodMs);});
    m_running = true;
    return true;
}

/***********************************************************************/
void DmxIs::join()
{
    m_running = false;
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

/***********************************************************************/
void DmxIs::setLed(uint8_t idx, uint8_t val)
{
    if (val < m_pMin) val = m_pMin;
    if (val > m_pMax) val = m_pMax;
    Lock lock(m_mutex);
    // Just for safety
    while ( m_ledsTx.size() <= idx)
        m_ledsTx.emplace_back(0);
    m_ledsTx.at(idx) = val;
}

/***********************************************************************/
void DmxIs::breathTest(const IndexSet& lines)
{
    Lock lock(m_mutex);
    m_breathTest = lines;
}

/***********************************************************************/
void DmxIs::setPowerRange(const uint8_t pMin, const uint8_t pMax)
{
    Lock lock(m_mutex);
    m_pMin = pMin;
    m_pMax = pMax;
}

/***********************************************************************/
void DmxIs::resetDevice()
{
    if (m_devh)
    {
        libusb_release_interface(m_devh, 0);
        libusb_close(m_devh);
        m_devh = nullptr;
    }
    m_sn = "N/C";
    m_idx = 0;
}

/***********************************************************************/
void DmxIs::tryReloadDevice()
{
    if (m_devh || !m_running) return;

    if (std::chrono::steady_clock::now() < m_retryCnx) return; // Do not retry too fast

    PRINT("R"); fflush(nullptr);
    m_retryCnx =  std::chrono::steady_clock::now() +  std::chrono::seconds(1);

    m_devh = libusb_open_device_with_vid_pid(m_ctx, m_vendor_id, m_product_id);

    if (!m_devh)
    {
        PRINT("Failed to bind DMXIS!\n");
        return; // failed (not plugged?)
    }

    printf("Found DMXIS!\n");
    m_idx = 0;
    // IMPORTANT FTDI
    if (libusb_kernel_driver_active(m_devh, 0) == 1)
    {
        libusb_detach_kernel_driver(m_devh, 0);
    }

    int r =libusb_claim_interface(m_devh, 0);

    if (r != 0)
    {
        printf("claim_interface failed: %d\n", r);
        throw std::exception();
    }

    // Handshakes and setups
    try
    {

        for (unsigned i{0}; i < BUFF90_NB_HDR; ++i)
        {
            readReg90();
        }


        vector<string> props;
        uint16_t nbFields{3};

        for (uint16_t iField{0}; iField < nbFields; iField++)
        {
            string sVal;
            const uint16_t hdr = readReg90();
            const uint8_t hType = (hdr >> 8);
            const uint8_t hLen = (hdr & 0xFF);

    //        cout<< "Read field #" << iField << ", HDR=" << std::hex << hdr<< ", Type=" <<(int)hType << ", len= " << (int)hLen <<endl;
            for (uint16_t iChar=0; iChar< hLen/2 -1 ; iChar++){
                uint16_t val = readReg90();
                if (hType == 0x03)
                {
                    const char c (val > 0x7F ? '?': (char)val);
                    sVal.push_back(c);
                }
            }
            props.emplace_back(sVal);
        }

        // Read SN
        m_sn.clear();
        while(1)
        {
            const uint16_t data = readReg90();
            if(!data) break;

            const char c (data > 0x7F ? '?': (char)data);
            m_sn.push_back(c);
        }

        props.emplace_back("SN=" + m_sn);

        printf("DMXIS:\n");
        for (const string& prop : props)
        {
            printf(" - %s\n", prop.c_str());
        }
        if (props.size() < 3)
        {
            throw runtime_error("Missing properties.");
        }
        if (props.at(0) != "ENTTEC")
        {
            throw runtime_error("Invalid VENDOR.");
        }
        if (props.at(1) != "DMXIS")
        {
            throw runtime_error("Invalid DEVICE ID.");
        }

        vector<ByteVect> handshakes={
                {0x7E,0x03,0x02,0x00,0x00,0x00,0xE7},
                {0x7E,0x0A,0x02,0x00,0x00,0x00,0xE7},
                {0x7e,0xfe,0x06,0x00,0x01,0x00,0xd6,0x2c,0x00,0x00,0xe7},
        };

        unsigned iHs{0};
        for (const auto& hs : handshakes)
        {
            iHs++;
            PRINT("Send handshake %u\n", iHs);
            {
                uint8_t rx[64];
                bulkWrite(hs);
                int sz = bulkRead(rx, (unsigned)sizeof(rx));

                PRINT("RX %d bytes\n", sz);
                for(int i=0;i<sz;i++)
                    PRINT("%02X ", rx[i]);
                PRINT("\n");
            }
        }
    }
    catch(const std::exception& e)
    {
        printf("Init of DMXIS failed : %s\n", e.what());
        resetDevice();
    }

}


/***********************************************************************/
void DmxIs::updateBreathValue(void)
{
   static float m_breathPhase{0.0f};
   const float range((m_pMax - m_pMin)/2);
   m_breathValue = m_pMin + (uint8_t)((1+cos(m_breathPhase)) * range) ;

   static_assert(DEFAULT_DMX_POLL_MS > 5);
   m_breathPhase += 3.1415 / DEFAULT_DMX_POLL_MS;
}

/***********************************************************************/
void DmxIs::threadImpl(int64_t sendPeriodMs)
{
    m_retryCnx =  std::chrono::steady_clock::now() +  std::chrono::milliseconds(100);

    try{
        while (m_running)
        {
            this_thread::sleep_for(chrono::milliseconds(sendPeriodMs));

            updateBreathValue();
            PRINT("BV=%d\n", (int)m_breathValue);

            try
            {
                tryReloadDevice();
                if (m_devh && m_running)
                {
                    static const ByteVect hdr{0x7e ,0x06 ,0x01, 0x02 ,0x00};
                    ByteVect tx{hdr};
                    {
                        Lock lock(m_mutex);
                        for (const uint8_t b: m_ledsTx)
                        {
                            tx.emplace_back(b);
                        }
                        for (const uint16_t idx: m_breathTest)
                        {
                            if (idx < NB_LEDS)
                            {
                                tx.at(hdr.size() + idx) = m_breathValue;
                            }
                        }
                    }
                    tx.emplace_back(0xe7);
                    bulkWrite(tx);
                }
            }
            catch (const UsbDeviceLostException& e)
            {
                printf("Device lost: %s\n", e.what());
                resetDevice();
            }
            catch (const std::exception& e) {
                printf("USB error: %s\n", e.what());
                m_nbFailures++;
                if (m_nbFailures > 10)
                    resetDevice();   // IMPORTANT
            }
        }
    }
    catch (const std::exception& e) {
        printf("DmxIs::threadImpl failed with exception: %s\n", e.what());
    }

    // On exit, stop all Leds
    if (m_devh)
    {
        try
        {
            ByteVect tx{0x7e ,0x06 ,0x01, 0x02 ,0x00};
            {
                for (uint16_t  i= 0; i < NB_LEDS ; i++) tx.emplace_back(m_pMin);
            }
            tx.emplace_back(0xe7);
            bulkWrite(tx);
        }
        catch (const std::exception& e) {
            printf("Could not stop LEDS : %s\n", e.what());
        }
    }
    m_running = false;
}

/***********************************************************************/
uint16_t DmxIs::readReg90()
{
    uint16_t data;
    uint16_t r = libusb_control_transfer(
            m_devh,
            0xC0,        // bmRequestType (IN | VENDOR | DEVICE)
            0x90,        // bRequest (0x90)
            0x0000,      // wValue
            m_idx,      // wIndex
            (unsigned char *) &data,
            2,
            1000
        );
    m_idx++;

    if ( r < 0)
    {
        if (r == LIBUSB_ERROR_NO_DEVICE)
        {
            throw UsbDeviceLostException("Device lost during control transfer");
        }

        throw std::exception();
    }
    return data;
}

/***********************************************************************/
void DmxIs::bulkWrite(const ByteVect& data, unsigned timeout)
{
    unsigned totalTransferred = 0;

    while (totalTransferred < data.size())
    {
        int transferred = 0;

        int r = libusb_bulk_transfer(
            m_devh,
            0x02,
            const_cast<unsigned char*>(data.data() + totalTransferred),
            data.size() - totalTransferred,
            &transferred,
            timeout
        );

        if (r != LIBUSB_SUCCESS)
        {
            PRINT("bulkWrite failed r=%d transferred=%d\n", r, transferred);

            if (r == LIBUSB_ERROR_NO_DEVICE ||
                r == LIBUSB_ERROR_IO ||
                r == LIBUSB_ERROR_PIPE)
            {
                resetDevice();
                throw UsbDeviceLostException("USB device disconnected during bulk write");
            }

            throw std::runtime_error("bulk write failed");
        }

        totalTransferred += transferred;

        if (transferred == 0)
        {
            throw std::runtime_error("bulk write stalled (0 bytes transferred)");
        }
    }
}

/***********************************************************************/
int DmxIs::bulkRead(uint8_t* buffer, unsigned maxSize, unsigned timeout)
{
    unsigned totalData = 0;

    while (totalData < maxSize)
    {
        uint8_t temp[1024]; // taille USB FTDI classique (adapter si besoin)
        int transferred = 0;

        int r = libusb_bulk_transfer(
            m_devh,
            0x81, // FTDI IN endpoint
            temp,
            (unsigned)sizeof(temp),
            &transferred,
            timeout
        );

        if (r != LIBUSB_SUCCESS)
        {
            if (r == LIBUSB_ERROR_NO_DEVICE)
                throw UsbDeviceLostException("Device lost during bulk read");
            throw std::runtime_error("bulk read failed");
        }

        if (transferred < 2)
        {
            // uniquement des octets de statut → on ignore
            continue;
        }

        int dataLen = transferred - 2;
        unsigned toCopy = std::min<int>(dataLen, maxSize - totalData);

        memcpy(buffer + totalData, temp + 2, toCopy);
        totalData += toCopy;

        if (toCopy == 0)
        {
            // pas de place restante ou pas de données utiles
            break;
        }
    }

    return totalData;

}

}  // namespace PBKR
