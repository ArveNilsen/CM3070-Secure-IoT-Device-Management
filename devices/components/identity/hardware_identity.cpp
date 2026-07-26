#include "identity/hardware_identity.hpp"

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
    return {};
}

std::expected<PublicKey, IdentityError>
HardwareIdentity::public_key(uint8_t slot) const
{
    return {};
}

std::expected<Signature, IdentityError>
HardwareIdentity::sign(uint8_t slot, std::span<const uint8_t> digest) const
{
#ifdef CONFIG_IDENTITY_STUB_MODE
    // Deterministic stub for testing
    Signature sig{};
    // mbedTLS software ECDSA sign
    return sig;
#else
    // TODO: Add real impl
    Signature sig{};
    return sig;
#endif
}

std::expected<Digest, IdentityError>
HardwareIdentity::firmware_hash(uint8_t slot) const
{
    return {};
}

bool HardwareIdentity::secure_boot_enabled() const
{
    return false;
}

bool HardwareIdentity::is_stub() const
{
    return false;
}
