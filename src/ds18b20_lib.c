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
#define SCRATCH_TEMP_FRACT_MASK(x) (((x & 0x08) ? 0.5 : 0.0) + ((x & 0x04) ? 0.25 : 0.0) + ((x & 0x02) ? 0.125 : 0.0) + ((x & 0x01) ? 0.0625 : 0.0))
#define SCRATCH_RESOLUTION_MASK(x) (x >> 5)
#define SCRATCH_CONFIG_MASK(x) ((x << 5) | 0x1F)

#define ROM_CRC 7
#define ROM_LEN 8

struct ds18b20_data {
  ds18b20_callback cb;
  struct mgos_ds18b20 *ds18b20;
};

struct mgos_ds18b20* ds18b20_create(uint8_t pin) {
  struct mgos_ds18b20* ds18b20 = calloc(1, sizeof(*ds18b20));
  /*
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
    free(ds18b20);
    return NULL;
  }

  cs_to_hex(data_str, rom, ROM_LEN); 
  LOG(LL_DEBUG, ("Found rom: 0x%s\r\n", data_str));

  crc = mgos_onewire_crc8(rom, ROM_LEN-1);
  LOG(LL_DEBUG, ("CRC: (0x%02X)\r\n", crc));

  if (crc != rom[ROM_CRC]) {
    LOG(LL_ERROR, ("CRC mismatch \r\n"));
    free(ds18b20);
    return NULL;
  }

  ds18b20->addr = calloc(ROM_LEN, sizeof(uint8_t));
  if (ds18b20->addr == NULL) {
    LOG(LL_ERROR, ("Cannot create storage for ROM\r\n"));
    free(ds18b20);
    return NULL;
  }
  memcpy(ds18b20->addr, rom, ROM_LEN);
  */
  return ds18b20;
}