#include "cryptoauthlib.h"
#include "esp_log.h"
#include "mbedtls/sha256.h"
#include <cstdio>
#include <array>

static const char* TAG = "provision";

// Injected at build time -- see provisioning/README.md
#ifndef FIRMWARE_SHA256_HEX
#error "FIRMWARE_SHA256_HEX must be defined at build time"
#endif

namespace {

constexpr uint8_t kKeySlot	= 0;
constexpr uint8_t kHashSlot = 8;

bool hex_decode32(const char* hex, uint8_t out[32])
{
		if (strlen(hex) != 64) return false;
		for (int i = 0; i < 32; ++i) {
				unsigned byte;
				if (sscanf(hex + i * 2, "%2x", &byte) != 1) return false;
				out[i] = static_cast<uint8_t>(byte);
		}

		return true;
}

void print_hex(const uint8_t* data, size_t len)
{
		for (size_t i = 0; i < len; ++i) printf("%02x", data[i]);
}

} // namespace

extern "C" void app_main()
{
		ATCAIfaceCfg cfg = {};
		cfg.iface_type = ATCA_I2C_IFACE;

		cfg.devtype = ATECC608B;

		cfg.atcai2c.address = 0xC0;
		cfg.atcai2c.bus = 0;
		cfg.atcai2c.baud = 100000;

		cfg.wake_delay = 1500;
		cfg.rx_retries = 20;

    ATCA_STATUS status = atcab_init(&cfg);
    if (status != ATCA_SUCCESS) {
        ESP_LOGE(TAG, "Chip init failed: 0x%02x", status);
        return;
    }

		// Chip serial number, used by capture.py
		uint8_t serial[9];
		status = atcab_read_serial_number(serial);
		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Failed to read serial number: 0x%02x", status);
				return;
		}

    // Generate key pair in the configured slot
    uint8_t public_key[64];
    status = atcab_genkey(kKeySlot, public_key);
    if (status != ATCA_SUCCESS) {
        ESP_LOGE(TAG, "Keygen failed: 0x%02x", status);
        return;
    }

		// Firmware hash
		uint8_t firmware_hash[32];
		if (!hex_decode32(FIRMWARE_SHA256_HEX, firmware_hash)) {
				ESP_LOGE(TAG, "Failed to decode FIRMWARE_SHA256_HEX");
				return;
		}

		status = atcab_write_zone(
				ATCA_ZONE_DATA, kHashSlot, 0, 0, firmware_hash, 32);
		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Hash write failed: 0x%02x", status);
				return;
		}

		// Emit a parseable line for capture.py
		printf("PROVISION_JSON:{");
		printf("\"chip_erial\":\"");
		print_hex(serial, sizeof(serial));
		printf("\",");
		printf("\"public_key\":\"");
		print_hex(public_key, sizeof(public_key));
		printf("\",");
		printf("\"firmware_hash\":\"%s\"m", FIRMWARE_SHA256_HEX);
		printf("\"firmware_version\":\"%s\",", IDF_VER);
		printf("\"key_slot\":%d,", kKeySlot);
		printf("\"hash_slot\":%d", kHashSlot);
		printf("}\n");

    ESP_LOGW(TAG, "About to lock slots. This is IRREVERSIBLE. "
                   "Confirm via serial before proceeding.");

    // TODO: Add gated confirmation
    //status = atcab_lock_config_zone();
    //status = atcab_lock_data_zone();
}
