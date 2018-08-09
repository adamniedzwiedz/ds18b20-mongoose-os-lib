# DS18B20 sensor library powering with an external supply


## Overview

[DS18B20](https://cdn.sparkfun.com/datasheets/Sensors/Temp/DS18B20.pdf) is a temperature sensor which works via One Wire interface. The sensor requires external resistor but there is [plenty modules](https://www.aliexpress.com/item/Free-Shipping-MLX90615-infrared-non-contact-temperature-measurement-sensor-module-IIC-communication/32242626689.html) which contains the necessary elements.

<p align="center">
  <img src="https://ae01.alicdn.com/kf/HTB1QcqIeGLN8KJjSZFKq6z7NVXat/DS18B20-single-bus-digital-temperature-sensor-module-for-Arduino.jpg">
</p> 

This is a simplest version of library which uses directly mongoose-os functions for onewire communication. The brain of the sensor is a scratchpad which is used to read a temperature and set a resolution (9-12 bit). Scratchpad also allows to set alarms but the library does NOT support *Alarm Search* command.

## Initialization

First, define which pin is used for one wire communication `ds18b20_create(uint8_t pin)`.
To read the scratchpad (read resolution or temperature) use `ds18b20_read_scratchpad(struct mgos_ds18b20* ds18b20, struct ds18b20_scratchpad* scratchpad)`
To write scratchpad (set resolution) use `ds18b20_write_scratchpad(struct mgos_ds18b20* ds18b20, struct ds18b20_scratchpad scratchpad)`.
At the end release the memory by calling `ds18b20_free(struct mgos_ds18b20* ds18b20)`

_Note_
The library supports only external supply and it is dedicated for modules. It does NOT support parasite-powered supply and alarm searches.

## Usage

```c
#include "mgos.h"
#include "ds18b20_lib.h"

#define ONEWIRE_PIN 14

static bool ds18b20_busy;
static struct mgos_ds18b20* ds18b20;

static void ds18b20_ready(void *arg) {
  float *temperature = arg;

  if (temperature == NULL) {
    LOG(LL_ERROR, ("Reading temperature has failed.\r\n"));
    return;
  }
  LOG(LL_INFO, ("Temperature: %f \r\n", *temperature));
  ds18b20_free(ds18b20);
  ds18b20_busy = false;
}

static void timer_cb(void *arg) {
  struct ds18b20_scratchpad scratchpad;

  if (ds18b20_busy) {
    return;
  } 

  ds18b20_busy = true;
  ds18b20 = ds18b20_create(ONEWIRE_PIN);

  if (ds18b20 == NULL) {
    LOG(LL_ERROR, ("DS18B20 is NULL\r\n"));
    return;
  }

  if (!ds18b20_read_scratchpad(ds18b20, &scratchpad)) {
    LOG(LL_ERROR, ("Reading scratchpad has failed\r\n"));
    return;
  }

  LOG(LL_INFO, ("Temperature: %f\r\n", scratchpad.temperature));
  LOG(LL_INFO, ("Alarm high: %d\r\n", scratchpad.alarm_high));
  LOG(LL_INFO, ("Alarm low: %d\r\n", scratchpad.alarm_low));
  LOG(LL_INFO, ("Resolution: %d\r\n", scratchpad.resolution));

  scratchpad.resolution = scratchpad.resolution == DS18B20_9_BIT ? DS18B20_12_BIT : DS18B20_9_BIT;

  if (!ds18b20_write_scratchpad(ds18b20, scratchpad)) {
    LOG(LL_ERROR, ("Writing scratchpad has failed\r\n"));
    return;
  }

  if (!ds18b20_start_conversion(ds18b20, ds18b20_ready)) {
    LOG(LL_ERROR, ("Starting conversion has failed\r\n"));
  }
  
  (void) arg;
}

enum mgos_app_init_result mgos_app_init(void) {
  mgos_set_timer(3000, MGOS_TIMER_REPEAT, timer_cb, NULL);
  return MGOS_APP_INIT_SUCCESS;
}
```

## Debugging

In case of any issues increase the debug level and check debug logs. 

```yaml
config_schema:
  - ["debug.level", 3]
```