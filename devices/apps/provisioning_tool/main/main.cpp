#include "cryptoauthlib.h"
#include "esp_log.h"

#include "firmware_hash.hpp" // auto-generated

#include "provisioning.hpp"

#include <cstdio>
#include <cstring>

#include "esp_log.h"

static const char* TAG = "provision";

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

constexpr size_t kWriteableOffset = 16;
constexpr size_t kWritableLength =
		provisioning::kConfigZoneWritable.size();

void log_config_bytes(const char* label, const uint8_t* data, size_t len)
{
		ESP_LOGI(TAG, "%s (%d bytes):", label, static_cast<int>(len));
		for (size_t i = 0; i < len; i += 8) {
				size_t remaining = (len - i < 8) ? (len - i) : 8;
				char line[64] = {0};
				char* p = line;
				for (size_t j = 0; j < remaining; ++j) {
						p += sprintf(p, "%02x ", data[i + j]);
				}
				ESP_LOGI(TAG, " [+%3d] %s", static_cast<int>(i), line);
		}
}

bool verify_readback(const uint8_t* intended, const uint8_t* actual, size_t len)
{
		bool all_match = true;
		int mismatch_count = 0;

		for (size_t i = 0; i < len; ++i) {
				if (intended[i] != actual[i]) {
						// Absolute offset, for cross-reference
						size_t abs_offset = kWriteableOffset + i;
						ESP_LOGE(TAG, "MISMATCH at config offset %d: "
												  "intended=0x%02x actual=0x%02x",
													static_cast<int>(abs_offset),
													intended[i], actual[i]);
						all_match = false;
						++mismatch_count;
				}
		}

		if (all_match) {
			ESP_LOGI(TAG, "Readback verification PASSED - "
										"all %d bytes match.", static_cast<int>(len));
		} else {
			ESP_LOGI(TAG, "Readback verification FAILED - "
										"%d bytes did not match.", mismatch_count);
		}

		return all_match;
}

bool write_and_verify_config_zone()
{
		// 1. Read current state
		// For logging, reference, debugging, etc.
		uint8_t before[128] = {0};
		ATCA_STATUS status = atcab_read_config_zone(before);
		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Pre-write config read failed: 0x%02x", status);
				return false;
		}

		log_config_bytes("Config zone BEFORE write", 
				before + kWriteableOffset, kWritableLength);

		// 2. Check lock status
		bool config_locked = false;
		status = atcab_is_locked(LOCK_ZONE_CONFIG, &config_locked);
		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Failed to check config lock state: 0x%02x", status);
				return false;
		}

		if (config_locked) {
				ESP_LOGW(TAG, "Config zone is ALREADY LOCKED - skipping write and "
											"proceeding to genkey.");
				return true;
		}

		// 3. Write config
		ESP_LOGI(TAG, "Writing config zone (offset %d, %d bytes)...",
				static_cast<int>(kWriteableOffset),
				static_cast<int>(kWritableLength));

		status = atcab_write_bytes_zone(
				ATCA_ZONE_CONFIG, 0, kWriteableOffset,
				provisioning::kConfigZoneWritable.data(), kWritableLength);

		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Config zone write failed: 0x%02x", status);
				return false;
		}

		// 4. Read back config zone and compare.
		// MUST match before locking, there is no return!
		uint8_t after[128] = {0};
		status = atcab_read_config_zone(after);
		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Post-write config read failed: 0x%02x", status);
				return false;
		}

		log_config_bytes("Config zone AFTER write", 
				after + kWriteableOffset, kWritableLength);

		// 5. Bytes 0-15 MUST be unchanged, verifying for safety.
		if (memcmp(before, after, kWriteableOffset) != 0) {
				ESP_LOGE(TAG, "UNEXPECTED: fixed region (bytes 0-15) "
											"changed after write! DO NOT PROCEED!");
				return false;
		}

		return verify_readback(
				provisioning::kConfigZoneWritable.data(),
				after + kWriteableOffset,
				kWritableLength);
}

bool inspect_and_confirm_lockable()
{
		// Read and confirm the following values:
		// SlotConfig[0]
		// KeyConfig[0]
		// SlotConfig[0]
		// KeyConfig[0]
		uint8_t config[128] = {0};
		ATCA_STATUS status = atcab_read_config_zone(config);
		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Config read failed: 0x%02x", status);
				return false;
		}

		ESP_LOGI(TAG, "SlotConfig[0] = %02x %02x (want: GenKey-capable ECC)",
				config[20], config[21]);
		ESP_LOGI(TAG, "KeyConfig[0] = %02x %02x (want: Private=1, ECC type)",
				config[96], config[97]);
		ESP_LOGI(TAG, "SlotConfig[8] = %02x %02x (want: plain data)",
				config[20 + 8*2], config[21 + 8*2]);
		ESP_LOGI(TAG, "KeyConfig[8] = %02x %02x (want: plain data)",
				config[96 + 8*2], config[97 + 8*2]);

		return true;
}

bool lock_config_after_review()
{
		// Run after manual datasheet cross-check.
		// inspect_and_confirm_lockable print this to monitor
		bool already_locked = false;
		ATCA_STATUS status = atcab_is_locked(LOCK_ZONE_CONFIG, &already_locked);
		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Lock-state check failed: 0x%02x", status);
				return false;
		}

		if (already_locked) {
				ESP_LOGI(TAG, "Config zone already locked - skipping");
				return true;
		}

		ESP_LOGW(TAG, "LOCKING CONFIG ZONE. This is IRREVERSIBLE.");

		status = atcab_lock_config_zone();
		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Config zone lock FAILED: 0x%02x", status);
				return false;
		}

		ESP_LOGI(TAG, "Config zone locked successfully.");
		return true;
}

} // namespace

extern "C" void app_main()
{
		ATCAIfaceCfg cfg = {};
		cfg.iface_type = ATCA_I2C_IFACE;
		cfg.atcai2c.address = 0xC0;
		cfg.devtype = ATECC608B;
		cfg.atcai2c.bus = 0;
		cfg.atcai2c.baud = 100000;
		cfg.wake_delay = 1500;
		cfg.rx_retries = 20;

    ATCA_STATUS status = atcab_init(&cfg);
    if (status != ATCA_SUCCESS) {
        ESP_LOGE(TAG, "Chip init failed: 0x%02x", status);
        return;
    }

		uint8_t serial[9] = {0};
		status = atcab_read_serial_number(serial);
		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Failed to read serial number: 0x%02x", status);
				return;
		}

		if (auto inspect = inspect_and_confirm_lockable(); !inspect)
				return;

		if (!lock_config_after_review()) {
				ESP_LOGE(TAG, "Config zone lock failed. Halting before genkey.");
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

		// Data zone lock is required for the devices firmware to read it back
		bool data_locked = false;
		status = atcab_is_locked(LOCK_ZONE_DATA, &data_locked);
		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Data zone lock check failed: 0x%02x", status);
				return;
		}
		if (!data_locked) {
				ESP_LOGW(TAG, "LOCKING DATA ZONE. This is IRREVERSIBLE.");
				status = atcab_lock_data_zone();
				if (status != ATCA_SUCCESS) {
						ESP_LOGE(TAG, "Data zone lock failed: 0x%02x", status);
						return;
				}
		}
		ESP_LOGI(TAG, "Data zone is locked.");

		// Emit a parseable line for capture.py
		printf("PROVISION_JSON:{");
		printf("\"chip_serial\":\"");
		print_hex(serial, sizeof(serial));
		printf("\",");
		printf("\"public_key\":\"");
		print_hex(public_key, sizeof(public_key));
		printf("\",");
		printf("\"firmware_hash\":\"%s\",", FIRMWARE_SHA256_HEX);
		printf("\"firmware_version\":\"%s\",", IDF_VER);
		printf("\"key_slot\":%d,", kKeySlot);
		printf("\"hash_slot\":%d", kHashSlot);
		printf("}\n");

    ESP_LOGI(TAG, "Provisioning complete.");
}
