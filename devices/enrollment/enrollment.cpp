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
}
