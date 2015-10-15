
#include "SensorHub.hpp"

using namespace std;

namespace mot {

#define VMM_ENTRY(reg, name, writable, addr, size) {#name, reg},
const map<string, uint16_t> SensorHub::Vmm = {
    #include "linux/motosh_vmm.h"
};
#undef VMM_ENTRY

int SensorHub::retryIoctl (int fd, int ioctl_number, ...) {
    va_list ap;
    void * arg;
    int status = 0;
    int error = 0;

    if (fd < 0) return fd;

    va_start(ap, ioctl_number);
    arg = va_arg(ap, void *);
    va_end(ap);

    do {
        status = ioctl(fd, ioctl_number, arg);
        error = errno;
    } while ((status != 0) && (error == -EINTR));
    return status;
}

int16_t SensorHub::getRegisterNumber(const string regName) {
    auto regEntry = Vmm.find(regName);
    if (regEntry == Vmm.end()) {
        return -1;
    }
    return static_cast<int16_t>(regEntry->second);
}


unique_ptr<uint8_t[]> SensorHub::readReg(VmmID vmmId, uint16_t size) {
    if (fd < 0) return nullptr;

    unique_ptr<uint8_t[]> res(new uint8_t[ max<uint16_t>(size, SENSORHUB_CMD_LENGTH) ]);

    uint16_t regNr = htons(static_cast<uint16_t>(vmmId));
    uint16_t dataSize = htons(size);
    memcpy(res.get(), &regNr, 2);
    memcpy(res.get() + 2, &dataSize, 2);

    int ret = retryIoctl(fd, SH_IOCTL_READ_REG, res.get());
    if (ret < 0) {
        return nullptr;
    } else {
        return res;
    }
}


unique_ptr<uint8_t[]> SensorHub::readReg(const string regName, uint16_t size) {
    auto regEntry = Vmm.find(regName);
    if (regEntry == Vmm.end()) {
        return nullptr;
    }

    return readReg(static_cast<VmmID>(regEntry->second), size);
}


bool SensorHub::writeReg(string regName, uint16_t size,
        const uint8_t * const data) {

    if (data == nullptr || size == 0) return false;

    int16_t regNr = getRegisterNumber(regName);
    if (regNr < 0) return false;

    regNr = EndianCvt<uint16_t>(regNr);
    uint16_t bytesLength = EndianCvt(size);

    char msg[SENSORHUB_CMD_LENGTH + size];
    memcpy(msg, &regNr, 2);
    memcpy(msg + 2, &bytesLength, 2);
    memcpy(msg + 4, data, size);

    int res = retryIoctl(fd, SH_IOCTL_WRITE_REG, msg);
    return res >= 0;
}

string SensorHub::getVariant(void) {
    char variantStr[FW_VERSION_SIZE] = {0};
    int res = retryIoctl(fd, SH_IOCTL_GET_VERNAME, variantStr);
    return res < 0 ? string() : variantStr;
}

string SensorHub::getVersionStr(void) {
    unique_ptr<uint8_t[]> verStr;

    unique_ptr<uint8_t[]> buff = readReg(VmmID::FW_VERSION_LEN, 1);
    if (!buff || buff[0] == 0) return string();

    verStr = readReg(VmmID::FW_VERSION_STR, buff[0] + 1); // read the \0
    return verStr ? (char *)verStr.get() : string();
}

uint32_t SensorHub::getFlashCrc(void) {
    unique_ptr<uint8_t[]> buff = readReg(VmmID::FW_CRC, 4);
    if (!buff) return 0;

    uint32_t hwCrc = Endian::extract<uint32_t>(&buff[0]);
    if (! isBigEndian) {
        // Extraction was done assuming data was BE, so we must swap.
        hwCrc = Endian::swap(hwCrc);
    }

    return hwCrc;
}

} // namespace mot

