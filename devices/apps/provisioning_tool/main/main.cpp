#include "cryptoauthlib.h"

inline constexpr std::string_view pTAG = "provision";

extern "C" void app_main()
{
    ATACAIfaceCfg cfg = cfg_ateccx08a_i2c_default;
    ATCA_STATUS status = atcab_init(&cfg);
    if (status != ATCA_SUCCESS) {
        ESP_LOGE(pTAG.data(), "Chip init failed: 0x%02x", status);
        return;
    }

    // Generate key pair in slot 0
    uint8_t public_key[64];
    status = atcab_genkey(0, public_key);
    if (status != ATCA_SUCCESS) {
        ESP_LOGE(pTAG.data(), "Keygen failed: 0x%02x", status);
        return;
    }

    // Print public key for manual registration with the gateway (out-of-band)
    ESP_LOGI(pTAG.data(), "Public key (register this with gateway):");
    for (int i = 0; i < 64; ++i)
        printf("%02X", public_key[i]);
    printf("\n");

    // Write firmware hash to a data slot (example here is slot 8)
    uint8_t firmware_hash[32] = { /* add precomputed SHA-256 of firmware */ };
    status = atcab_write_zone(ATCA_ZONE_DATA, 8, 0, 0, firmware_hash, 32);
    if (status != ATCA_SUCCESS) {
        ESP_LOGE(pTAG.data(), "Hash write failed: 0x%02X", status);
        return;
    }

    ESP_LOGW(pTAG.data(), "About to lock slots. This is IRREVERSIBLE. "
                          "Confirm via serial before proceeding.");

    // TODO: Add gated confirmation
    //status = atcab_lock_config_zone();
    //status = atcab_lock_data_zone();
}
