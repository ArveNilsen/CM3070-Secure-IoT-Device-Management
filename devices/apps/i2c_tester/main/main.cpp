/*
 * Adapted from the i2c examples from EspressIf.
 * Small app for testing that the i2c is wired up correctly.
 */

#include "driver/i2c.h"

void i2c_scan()
{
    for (uint8_t addr = 1; addr < 127; ++addr) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();

        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t result = i2c_master_cmd_begin(
            I2C_NUM_0, cmd, pdMS_TO_TICKS(50));

        i2c_cmd_link_delete(cmd);

        if (result == ESP_OK) 
            ESP_LOGI("scan", "Found device at 0x%02X", addr);
    }
}
