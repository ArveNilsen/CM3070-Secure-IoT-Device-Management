#include "capability/capability_enforcement.hpp"

#include "esp_log.h"

static const char* TAG = "CAPABILITY_ENFORCER";

using dev::CapabilityEnforcer;
using result_type = dev::CapabilityEnforcer::result_type;

CapabilityEnforcer::CapabilityEnforcer(DeviceConfig& config) 
    : config_{config} {}

result_type CapabilityEnforcer::check(Capability cap) const
{
    return {};
}

result_type CapabilityEnforcer::apply_restriction(uint32_t permitted_mask)
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
    return {};
}

void CapabilityEnforcer::quarantine()
{
    return;
}

result_type CapabilityEnforcer::restore_to_ceiling()
{
    return {};
}

uint32_t CapabilityEnforcer::active_capabilities() const
{
    return 0;
}

uint32_t CapabilityEnforcer::manifest_capabilities() const
{
    return 0;
}

result_type CapabilityEnforcer::load_manifest()
{
    return {};
}

