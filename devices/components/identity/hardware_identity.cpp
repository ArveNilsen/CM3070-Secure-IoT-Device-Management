std::expected<Signature, IdentityError>
HardwareIdentity::sign(uint8_t slot, std::span<const uint8_t digest) const
{
#ifdef CONFIG_IDENTITY_STUB_MODE
    // Deterministic stub for testing
    Signature sig{};
    // mbedTLS software ECDSA sign
    return sig;
#else
    // Real impl
#endif
}
