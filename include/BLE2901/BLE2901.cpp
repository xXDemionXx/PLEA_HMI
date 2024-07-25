#include "BLE2901.h"

BLE2901::BLE2901() : BLEDescriptor(BLEUUID((uint16_t)0x2901)) {
    // Set initial value if needed
}

void BLE2901::setValue(const std::string &value) {
    BLEDescriptor::setValue((uint8_t*)value.data(), value.length());
}
