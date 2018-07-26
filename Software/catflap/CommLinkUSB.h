#ifndef COMMLINKUSB_H_
#define COMMLINKUSB_H_

#include "common_defs.h"

struct libusb_device_handle;

class CommLinkUSB {
public:
    CommLinkUSB();
    ~CommLinkUSB();

    bool Initialize();

    int SendByte(uint8 data, uint32 timeout);
    int ReceiveByte(uint8* data, uint32 timeout);
    int Flush(uint32 timeout);

    int DiscardBytes(size_t count, uint32 timeout);

private:
    int FlushIfFull(uint32 timeout);

    enum { MAX_CACHED_BYTES = 64 };

    int bytesCachedCount;
    uint8 cachedBytes[MAX_CACHED_BYTES];

    libusb_device_handle* deviceHandle;
};


#endif
