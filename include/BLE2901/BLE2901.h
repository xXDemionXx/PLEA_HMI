#ifndef _BLE2901_H_
#define _BLE2901_H_

#include <BLEDescriptor.h>

class BLE2901 : public BLEDescriptor {
public:
    BLE2901();
    void setValue(const std::string &value);
};

#endif
