#include "identity/hardware_identity.hpp"
#include "cryptoauthlib.h"

#include "esp_log.h"

using dev::HardwareIdentity;
using dev::Signature;
using dev::IdentityError;
using dev::Digest;
using dev::PublicKey;

static const char* TAG = "HW_IDENTITY";

HardwareIdentity& HardwareIdentity::instance()
{
    static HardwareIdentity hi;
    return hi;
}

static ATCAIfaceCfg cfg = {};

std::expected<void, IdentityError> HardwareIdentity::init()
{
		cfg.iface_type = ATCA_I2C_IFACE;
		cfg.atcai2c.address = 0xC0;
		cfg.devtype = ATECC608B;
		cfg.atcai2c.bus = 0;
		cfg.atcai2c.baud = 100000;
		cfg.wake_delay = 1500;
		cfg.rx_retries = 20;

    ATCA_STATUS status = atcab_init(&cfg);
    if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "atcab_init() failed.");
        return std::unexpected(IdentityError::ChipNotFound);
		}

    stub_mode_ = false;
		ESP_LOGI(TAG, "Init complete");
		
		bool data_locked = false;
		status = atcab_is_locked(LOCK_ZONE_DATA, &data_locked);
		if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "Data zone lock check failed: 0x%02x", status);
        return std::unexpected(IdentityError::InvalidSlot);
		}
		if (!data_locked) {
				ESP_LOGW(TAG, "DATA ZONE IS NOT LOCKED");
		} else {
			ESP_LOGI(TAG, "Data zone is locked.");
		}

		auto digest = firmware_hash(8);
		if (!digest)
        return std::unexpected(IdentityError::InvalidSlot);

    Digest hash{};
    status = atcab_read_zone(ATCA_ZONE_DATA, 8, 0, 0,
        hash.data(), DIGEST_SIZE);

    if (status != ATCA_SUCCESS)
        return std::unexpected(IdentityError::InvalidSlot);

		digest_ = hash;

    return {};
}

std::expected<PublicKey, IdentityError>
HardwareIdentity::public_key(uint8_t slot) const
{
    PublicKey pub{};
    ATCA_STATUS status = atcab_get_pubkey(slot, pub.data());
    if (status != ATCA_SUCCESS)
        return std::unexpected(IdentityError::InvalidSlot);
    return pub;
}

std::expected<Signature, IdentityError>
HardwareIdentity::sign(uint8_t slot, std::span<const uint8_t> digest) const
{
    if (digest.size() != DIGEST_SIZE)
        return std::unexpected(IdentityError::SigningFailed);

    Signature sig{};
    ATCA_STATUS status = atcab_sign(slot, digest.data(), sig.data());
    
    if (status != ATCA_SUCCESS) {
				ESP_LOGE(TAG, "atcab_sign failed: 0x%02x", status);
        return std::unexpected(IdentityError::SigningFailed);
		}

    return sig;
}

std::expected<Digest, IdentityError>
HardwareIdentity::firmware_hash(uint8_t slot) const
{
#if 0
    Digest hash{};
    status = atcab_read_zone(ATCA_ZONE_DATA, slot, 0, 0,
        hash.data(), DIGEST_SIZE);

    if (status != ATCA_SUCCESS)
        return std::unexpected(IdentityError::InvalidSlot);
    return hash;
#endif
		return digest_;
}

bool HardwareIdentity::secure_boot_enabled() const
{
    return false;
}

bool HardwareIdentity::is_stub() const
{
    return false;
}
