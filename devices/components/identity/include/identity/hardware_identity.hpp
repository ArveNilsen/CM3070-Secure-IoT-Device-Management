#pragma once

// Standard library imcludes
#include <span>
#include <array>
#include <expected>
#include <cstdint>

// ESP-IDF includes
#include "esp_err.h"

namespace dev {

enum class IdentityError {
    ChipNotSigned,
    SigningFailed,
    SlotLocked,
    InvalidSlot,
    StubMode // chip is absent
};

constexpr esp_err_t to_esp_err(IdentityError e) noexcept 
{
    switch (e) {
        case IdentityError::ChipNotSigned:  return ESP_ERR_INVALID_STATE;
        case IdentityError::SigningFailed:  return ESP_ERR_INVALID_ARG;
        case IdentityError::SlotLocked:     return ESP_ERR_NOT_ALLOWED;
        case IdentityError::InvalidSlot:    return ESP_ERR_NOT_SUPPORTED;
        case IdentityError::StubMode:       return ESP_ERR_INVALID_VERSION;
    }
    return ESP_FAIL; // unreachable
}

// Fixed sizes for the ATEC608A ECDSA P-256
constexpr size_t PUBLIC_KEY_SIZE = 64;
constexpr size_t SIGNATURE_SIZE  = 64;
constexpr size_t DIGEST_SIZE     = 32;

using PublicKey = std::array<uint8_t, PUBLIC_KEY_SIZE>;
using Signature = std::array<uint8_t, SIGNATURE_SIZE>;
using Digest    = std::array<uint8_t, DIGEST_SIZE>;

class HardwareIdentity {
public:
    static HardwareIdentity& instance();

    // Initialise I2C and verify chip presence
    std::expected<void, IdentityError> init();

    // Read public key from key slot
    std::expected<PublicKey, IdentityError> public_key(uint8_t slot) const;

    // Sign a digest
    std::expected<Signature, IdentityError>
        sign(uint8_t slot, std::span<const uint8_t> digest) const;

    // Read firmware hash from locked data slot
    std::expected<Digest, IdentityError>
        firmware_hash(uint8_t slot) const;

    // Read secure boot status from ESP32 eFuse
    bool secure_boot_enabled() const;

    // True if not chip present
    bool is_stub() const;

private:
    HardwareIdentity() = default;
    bool stub_mode_ = false;
};

} //namespace dev
