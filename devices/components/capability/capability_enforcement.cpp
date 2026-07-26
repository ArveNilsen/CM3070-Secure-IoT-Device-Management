std::expected<void, EnforcementError>
CapabilityEnforcer::apply_restriction(uint32_t permitted_mask)
{
    // Invariant ActiveSubset
    const uint32_t bounded = permitted_mask & ceiling_mask_;
    if (bounded != permitted_mask) {
        // Gateway sent a mask exceeding ceiling.
        // Apply the intersection silently and log warning.
        // TODO: Consider letting the gateway know.
        ESP_LOGW(TAG, "Restrition mask exceeds ceiling, intersection applied");
    }

    active_mask_ = bounded;
    return {}
}
