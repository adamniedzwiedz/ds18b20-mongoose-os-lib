#include "mgos.h"

#ifndef DS18B20_LIB_H
#define DS18B20_LIB_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ds18b20 measure callback */
typedef void (*ds18b20_callback)(void *param);

enum ds18b20_resolution {
  DS18B20_9_BIT  = 0,
  DS18B20_10_BIT = 1,
  DS18B20_11_BIT = 2,
  DS18B20_12_BIT = 3,
};

struct mgos_ds18b20;

struct ds18b20_scratchpad {
  float temperature;
  int alarm_high;
  int alarm_low;
  enum ds18b20_resolution resolution;
};

struct mgos_ds18b20* ds18b20_create(uint8_t pin);
//struct ds18b20_scratchpad* ds18b20_read_scratchpad(struct mgos_ds18b20* ds18b20);
//bool ds18b20_write_scratchpad(struct mgos_ds18b20* ds18b20, struct ds18b20_scratchpad scratchpad);
//bool ds18b20_start_conversion(struct mgos_ds18b20* ds18b20, ds18b20_callback ds18b20_cb);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DS18B20_LIB_H */