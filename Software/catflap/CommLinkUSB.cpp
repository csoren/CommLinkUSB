#include <libusb-1.0/libusb.h>
#include "CommLinkUSB.h"
#include "CCaetlaDefs.h"

#define VENDOR_ID  0x1209
#define PRODUCT_ID 0xBAD2

#define ENDPOINT_IN  (0x03 | LIBUSB_ENDPOINT_IN)
#define ENDPOINT_OUT (0x04 | LIBUSB_ENDPOINT_OUT)

CommLinkUSB::CommLinkUSB() 
:   deviceHandle(NULL),
    bytesCachedCount(0) {
    libusb_init(NULL);
}

CommLinkUSB::~CommLinkUSB() {
    if (deviceHandle != NULL)
        libusb_close(deviceHandle);

    libusb_exit(NULL);
}


static int USBErrorToCaetla(int result, bool lengthOk) {
    if (result == 0 && lengthOk)
        return CAETLA_ERROR_OK;

    if (result == LIBUSB_ERROR_TIMEOUT)
        return CAETLA_ERROR_TIMEOUT;

    if (result == LIBUSB_ERROR_NO_DEVICE)
        return CAETLA_ERROR_DEVICE_NOT_PRESENT;

    return CAETLA_ERROR_UNKNOWN;
}


bool CommLinkUSB::Initialize() {
    deviceHandle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    if (deviceHandle != NULL) {
        if (libusb_set_configuration(deviceHandle, 1) != 0)
            return false;
        int result = libusb_claim_interface(deviceHandle, 0);
        return result == 0;
    }
    return false;
}


int CommLinkUSB::SendByte(uint8 data, uint32 timeout) {
    int error = FlushIfFull(timeout);
    if (error != CAETLA_ERROR_OK)
        return error;

    cachedBytes[bytesCachedCount++] = data;
    return CAETLA_ERROR_OK;
}

int CommLinkUSB::ReceiveByte(uint8* data, uint32 timeout) {
    Flush(timeout);

    int actualLength;
    int result = libusb_bulk_transfer(deviceHandle, ENDPOINT_IN, data, 1, &actualLength, timeout);
    return USBErrorToCaetla(result, actualLength == 1);
}

int CommLinkUSB::FlushIfFull(uint32 timeout) {
    if (bytesCachedCount == MAX_CACHED_BYTES)
        return Flush(timeout);

    return CAETLA_ERROR_OK;
}

int CommLinkUSB::Flush(uint32 timeout) {
    if (bytesCachedCount > 0) {
        int actualLength;
        int result = libusb_bulk_transfer(deviceHandle, ENDPOINT_OUT, cachedBytes, bytesCachedCount, &actualLength, timeout);
        int caetlaError = USBErrorToCaetla(result, actualLength == bytesCachedCount);
        bytesCachedCount = 0;
        return caetlaError;
    }

    return CAETLA_ERROR_OK;
}

int CommLinkUSB::DiscardBytes(size_t count, uint32 timeout) {
    Flush(timeout);

    while (count-- > 0) {
        uint8 data;
        int error = ReceiveByte(&data, timeout);
        if (error != CAETLA_ERROR_OK)
            return error;
    }

    return CAETLA_ERROR_OK;
}
