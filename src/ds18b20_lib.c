#include "mgos.h"
#include "mgos_onewire.h"
#include "mgos_system.h"
#include "common/str_util.h"
#include "ds18b20_lib.h"

// DS18B20 onewire commands
#define CONVERT_CMD         0x44  // Start temperature conversion
#define COPYSCRATCH_CMD     0x48  // Copy EEPROM
#define READSCRATCH_CMD     0xBE  // Read EEPROM
#define WRITESCRATCH_CMD    0x4E  // Write to EEPROM

// Scratchpad locations
#define SCRATCH_TEMP_LSB        0
#define SCRATCH_TEMP_MSB        1
#define SCRATCH_ALARM_HIGH      2
#define SCRATCH_ALARM_LOW       3
#define SCRATCH_CONFIG          4
#define SCRATCH_CRC             8

#define SCRATCH_LEN 9

#define SCRATCH_SIGN_MASK(x) (x & 0xF8)
#define SCRATCH_TEMP_MSB_MASK(x)  ((x & 0x07) << 4)
#define SCRATCH_TEMP_LSB_SHIFT(x) (x >> 4) 
// avoid multiplication it causes numeric errors
#define SCRATCH_TEMP_FRACT_MASK(x) (((x & 0x08) ? 0.5 : 0.0) + ((x & 0x04) ? 0.25 : 0.0) + ((x & 0x02) ? 0.125 : 0.0) + ((x & 0x01) ? 0.0625 : 0.0))
#define SCRATCH_RESOLUTION_MASK(x) (x >> 5)
#define SCRATCH_CONFIG_MASK(x) ((x << 5) | 0x1F)

#define ROM_CRC 7
#define ROM_LEN 8

struct ds18b20_data {
  ds18b20_callback cb;
  struct mgos_ds18b20 *ds18b20;
};

struct mgos_ds18b20 {
  struct mgos_onewire* one_wire;
  uint8_t* addr;
};

void ds18b20_free(struct mgos_ds18b20* ds18b20) {
  if (ds18b20 != NULL) {
    if (ds18b20->addr != NULL) {
      free(ds18b20->addr);
      ds18b20->addr = NULL;
    }
    if (ds18b20->one_wire != NULL) {
      free(ds18b20->one_wire);
      ds18b20->one_wire = NULL;
    }
    free(ds18b20);
  } 
}

bool check_params(struct mgos_ds18b20* ds18b20) {
  if (ds18b20 == NULL) {
    LOG(LL_ERROR, ("parameter ds18b20 cannot be NULL\r\n"));
    return false;
  }

  if (ds18b20->addr == NULL) {
    LOG(LL_ERROR, ("parameter ds18b20 has no address\r\n"));
    return false;
  }

  if (ds18b20->one_wire == NULL) {
    LOG(LL_ERROR, ("parameter ds18b20 has no one wire object\r\n"));
    return false;
  }
  return true;
}

struct mgos_ds18b20* ds18b20_create(uint8_t pin) {
  struct mgos_ds18b20* ds18b20 = calloc(1, sizeof(*ds18b20));
  
  char data_str[2*ROM_LEN+1] = {0}; 
  uint8_t rom[ROM_LEN];
  uint8_t crc;

  if (ds18b20 == NULL) {
    LOG(LL_ERROR, ("Cannot create mgos_ds18b20 structure.\r\n"));
    return NULL;
  }

  ds18b20->one_wire = mgos_onewire_create(pin);

  if (ds18b20->one_wire == NULL) {
    LOG(LL_ERROR, ("Cannot create mgos_onewire structure.\r\n"));
    free(ds18b20);
    return NULL;
  }
  
  LOG(LL_DEBUG, ("Reset search\r\n"));
  mgos_onewire_search_clean(ds18b20->one_wire);
  
  LOG(LL_DEBUG, ("Search device...\r\n"));
  if (!mgos_onewire_next(ds18b20->one_wire, rom, 0)) {
    LOG(LL_ERROR, ("Onewire device has not been found!\r\n"));
    ds18b20_free(ds18b20);
    return NULL;
  }

  cs_to_hex(data_str, rom, ROM_LEN); 
  LOG(LL_DEBUG, ("Found rom: 0x%s\r\n", data_str));

  crc = mgos_onewire_crc8(rom, ROM_LEN-1);
  LOG(LL_DEBUG, ("CRC: (0x%02X)\r\n", crc));

  if (crc != rom[ROM_CRC]) {
    LOG(LL_ERROR, ("CRC mismatch \r\n"));
    ds18b20_free(ds18b20);
    return NULL;
  }

  ds18b20->addr = calloc(ROM_LEN, sizeof(uint8_t));
  if (ds18b20->addr == NULL) {
    LOG(LL_ERROR, ("Cannot create storage for ROM\r\n"));
    ds18b20_free(ds18b20);
    return NULL;
  }
  memcpy(ds18b20->addr, rom, ROM_LEN);
  
  return ds18b20;
}

bool ds18b20_read_scratchpad(struct mgos_ds18b20* ds18b20, struct ds18b20_scratchpad* scratchpad) {
  char data_str[2*SCRATCH_LEN+1] = {0}; 
  uint8_t buf[SCRATCH_LEN];
  uint8_t crc;
  int8_t temp_int;
  float temp_frac;

  if (!check_params(ds18b20)) {
    return false;
  }

  if (scratchpad == NULL) {
    LOG(LL_ERROR, ("parameter scratchpad cannot be NULL.\r\n"));
    return false;
  }

  if (mgos_onewire_reset(ds18b20->one_wire) == 0) {
    LOG(LL_ERROR, ("Reseting the Data line has failed.\r\n"));
    return false;
  }

  LOG(LL_DEBUG, ("Sending read scratch (0x%02X) command\r\n", READSCRATCH_CMD));
  mgos_onewire_select(ds18b20->one_wire, ds18b20->addr);
  mgos_onewire_write(ds18b20->one_wire, READSCRATCH_CMD);
  
  LOG(LL_DEBUG, ("Reading bytes...\r\n"));
  mgos_onewire_read_bytes(ds18b20->one_wire, buf, SCRATCH_LEN);
  
  cs_to_hex(data_str, buf, SCRATCH_LEN); 
  LOG(LL_DEBUG, ("Read scratchpad: %s\r\n", data_str));

  crc = mgos_onewire_crc8(buf, SCRATCH_LEN-1);
  LOG(LL_DEBUG, ("CRC: (0x%02X)\r\n", crc));

  if (crc != buf[SCRATCH_CRC]) {
    LOG(LL_ERROR, ("CRC mismatch \r\n"));
    return false;
  }

  if (mgos_onewire_reset(ds18b20->one_wire) == 0) {
    LOG(LL_ERROR, ("Reseting the Data line at the end has failed.\r\n"));
    return false;
  }

  temp_int = (SCRATCH_TEMP_MSB_MASK(buf[SCRATCH_TEMP_MSB]) + SCRATCH_TEMP_LSB_SHIFT(buf[SCRATCH_TEMP_LSB]));
  // avoid multiplication it causes numeric errors
  if (SCRATCH_SIGN_MASK(buf[SCRATCH_TEMP_MSB])) {
    temp_int = -temp_int;
  }

  temp_frac = SCRATCH_TEMP_FRACT_MASK(buf[SCRATCH_TEMP_LSB]);
  LOG(LL_DEBUG, ("integer temp: %d\r\n", temp_int));
  LOG(LL_DEBUG, ("frac temp: %f\r\n", temp_frac));
  scratchpad->temperature = (float)temp_int + temp_frac;
  scratchpad->alarm_high = buf[SCRATCH_ALARM_HIGH];
  scratchpad->alarm_low = buf[SCRATCH_ALARM_LOW];
  scratchpad->resolution = SCRATCH_RESOLUTION_MASK(buf[SCRATCH_CONFIG]);

  return true;
}

bool ds18b20_write_scratchpad(struct mgos_ds18b20* ds18b20, struct ds18b20_scratchpad scratchpad) {
  if (!check_params(ds18b20)) {
    return false;
  }

  if (mgos_onewire_reset(ds18b20->one_wire) == 0) {
    LOG(LL_ERROR, ("Reseting the Data line has failed.\r\n"));
    return false;
  }

  LOG(LL_DEBUG, ("Sending write scratch (0x%02X) command\r\n", WRITESCRATCH_CMD));
  mgos_onewire_select(ds18b20->one_wire, ds18b20->addr);
  mgos_onewire_write(ds18b20->one_wire, WRITESCRATCH_CMD);
  mgos_onewire_write(ds18b20->one_wire, scratchpad.alarm_high);
  mgos_onewire_write(ds18b20->one_wire, scratchpad.alarm_low);
  mgos_onewire_write(ds18b20->one_wire, SCRATCH_CONFIG_MASK(scratchpad.resolution));

  LOG(LL_DEBUG, ("Scratchpad has been written\r\n"));

  if (mgos_onewire_reset(ds18b20->one_wire) == 0) {
    LOG(LL_ERROR, ("Another reseting the Data line has failed.\r\n"));
    return false;
  }

  if (mgos_onewire_reset(ds18b20->one_wire) == 0) {
    LOG(LL_ERROR, ("Reseting the Data line has failed.\r\n"));
    return false;
  }

  LOG(LL_DEBUG, ("Sending copy scratch (0x%02X) command\r\n", COPYSCRATCH_CMD));
  mgos_onewire_select(ds18b20->one_wire, ds18b20->addr);
  mgos_onewire_write(ds18b20->one_wire, COPYSCRATCH_CMD);
  mgos_msleep(10);

  if (mgos_onewire_reset(ds18b20->one_wire) == 0) {
    LOG(LL_ERROR, ("Reseting the Data line at the end has failed.\r\n"));
    return false;
  }
  return true;
}

static void conversion_cb(void *arg) {
  struct ds18b20_data* data = arg;
  struct ds18b20_scratchpad scratchpad;

  if (data == NULL) {
    LOG(LL_ERROR, ("NULL data argument in conversion callback.\r\n"));
    return;
  }
  if (data->ds18b20 == NULL) {
    LOG(LL_ERROR, ("NULL ds18b20 argument in conversion callback.\r\n"));
    free(data);
    return;
  }
  if (data->ds18b20->one_wire == NULL) {
    LOG(LL_ERROR, ("NULL one_wire argument in conversion callback.\r\n"));
    free(data);
    return;
  }
  if (!mgos_onewire_read_bit(data->ds18b20->one_wire)) {
    mgos_set_timer(100 /* ms */, false /* repeat */, conversion_cb, data);
  } else if (data->cb != NULL) {
    if (!ds18b20_read_scratchpad(data->ds18b20, &scratchpad)) {
      LOG(LL_ERROR, ("Reading scratchpad has failed\r\n"));
      free(data);
      return;
    }
    data->cb(&scratchpad.temperature);
    free(data);
  }
}

bool ds18b20_start_conversion(struct mgos_ds18b20* ds18b20, ds18b20_callback ds18b20_cb) {
  struct ds18b20_data* data;
  
  if (ds18b20_cb == NULL) {
    LOG(LL_ERROR, ("parameter ds18b20_callback cannot be null\r\n"));
    return false;
  }

  data = calloc(1, sizeof(*data));

  if (data == NULL) {
    LOG(LL_ERROR, ("Cannot create ds18b20 data structure\r\n"));
    return false;
  }

  if (!check_params(ds18b20)) {
    free(data);
    return false;
  }
  
  if (mgos_onewire_reset(ds18b20->one_wire) == 0) {
    LOG(LL_ERROR, ("Reseting the Data line has failed.\r\n"));
    free(data);
    return false;
  }

  LOG(LL_DEBUG, ("Sending convert (0x%02X) command\r\n", CONVERT_CMD));
  mgos_onewire_select(ds18b20->one_wire, ds18b20->addr);
  mgos_onewire_write(ds18b20->one_wire, CONVERT_CMD);

  data->ds18b20 = ds18b20;
  data->cb = ds18b20_cb;
  mgos_set_timer(100 /* ms */, false /* repeat */, conversion_cb, data);
  return true;
}