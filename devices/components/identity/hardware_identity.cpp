#include "identity/hardware_identity.hpp"
#include "cryptoauthlib.h"

using dev::HardwareIdentity;
using dev::Signature;
using dev::IdentityError;
using dev::Digest;
using dev::PublicKey;

HardwareIdentity& HardwareIdentity::instance()
{
    static HardwareIdentity hi;
    return hi;
}

std::expected<void, IdentityError> HardwareIdentity::init()
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
        return std::unexpected(IdentityError::ChipNotFound);
		}

		uint8_t revision[4];
		status = atcab_info(revision);
		if (status != ATCA_SUCCESS) {
        return std::unexpected(IdentityError::ChipNotFound);
		}

    stub_mode_ = false;
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
    
    if (status != ATCA_SUCCESS)
        return std::unexpected(IdentityError::SigningFailed);

    return sig;
}

std::expected<Digest, IdentityError>
HardwareIdentity::firmware_hash(uint8_t slot) const
{
    Digest hash{};
    ATCA_STATUS status = atcab_read_zone(ATCA_ZONE_DATA, slot, 0, 0,
        hash.data(), DIGEST_SIZE);

    if (status != ATCA_SUCCESS)
        return std::unexpected(IdentityError::InvalidSlot);
    return hash;
}

bool HardwareIdentity::secure_boot_enabled() const
{
    return false;
}

bool HardwareIdentity::is_stub() const
{
    return false;
}
