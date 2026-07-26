#include "enrollment/enrollment.hpp"

using dev::EnrollmentService;
using dev::EnrollmentResult;
using dev::EnrollmentError;

EnrollmentService::EnrollmentService( DeviceConfig& config,
    HardwareIdentity& identity)
    : config_{config}, identity_{identity} {}

std::expected<EnrollmentResult, EnrollmentError> EnrollmentService::enroll()
{
    return {};
}

bool EnrollmentService::is_enrolled() const
{
    return false;
}

std::expected<std::vector<uint8_t>, EnrollmentError> 
EnrollmentService::request_nonce()
{
    return {};
}

std::expected<std::vector<uint8_t>, EnrollmentError>
EnrollmentService::build_attestation(std::span<const uint8_t> nonce)
{
/*
 * Attestation evidence as serialisable structure.
AttestationEvidence {
    public_key_id   : string
    nonce           : bytes
    timestamp       : uint64
    firmware_hash   : bytes[32]
    device_class    : string
    secure_boot     : bool
    signature       : bytes[64]  ← ECDSA over all fields above
}
Add CBOR as dependency
*/
    return {};
}

// Phase 3: submit evidence, receive manifest
std::expected<EnrollmentResult, EnrollmentError>
EnrollmentService::submit_attestation(std::span<const uint8_t> evidence)
{
    return {};
}

// Phase 4: verify gateway signature over manifest
std::expected<void, EnrollmentError>
EnrollmentService::verify_manifest(const EnrollmentResult& result)
{
    return {};
}

