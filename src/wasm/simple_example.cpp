#include <stdint.h>

extern "C" {
    void setUint32(const char* name, uint32_t value);
  
    void executeBlock() {
        setUint32("foobar", 100);
    }
}