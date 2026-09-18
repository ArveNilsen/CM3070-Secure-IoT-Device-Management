#pragma once

#include <expected>
#include <functional>
#include <cstdint>
#include <span>

#include "configs/device_config.hpp"
#include "identity/hardware_identity.hpp"

namespace dev {

class DeviceConfig;
class HardwareIdentity;

enum class EnrollmentError {
    AlreadyEnrolled,
    NetworkFailure,
    NonceTimeout,
    AttestationFailed,
    ManifestInvalid,
    SignatureInvalid,
    GatewayRejected,
		NotConfigured
};

struct EnrollmentResult {
    std::vector<uint8_t> manifest;
    std::vector<uint8_t> gateway_signature;
    uint32_t             manifest_version;
};

class EnrollmentService {
public:
    explicit EnrollmentService(
        DeviceConfig&     config,
        HardwareIdentity& identity);

    // Execute full enrollment protocol.
    // Blocks until enrolled, failed, or timed out.
    std::expected<EnrollmentResult, EnrollmentError> enroll();

    // Check if device is already enrolled
    bool is_enrolled() const;

private:
    // Phase 1: announce and request nonce
    std::expected<std::vector<uint8_t>, EnrollmentError> request_nonce();

    // Phase 2: construct attestation evidence
    std::expected<std::vector<uint8_t>, EnrollmentError>
        build_attestation(std::span<const uint8_t> nonce);

    // Phase 3: submit evidence, receive manifest
    std::expected<EnrollmentResult, EnrollmentError>
        submit_attestation(std::span<const uint8_t> evidence);

    // Phase 4: verify gateway signature over manifest
    std::expected<void, EnrollmentError>
        verify_manifest(const EnrollmentResult& result);

    DeviceConfig&     config_;
    HardwareIdentity& identity_;
};

} // namespace dev
