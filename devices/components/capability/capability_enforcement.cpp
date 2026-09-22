#include "capability/capability_enforcement.hpp"
#include "configs/device_config.hpp"
#include "esp_log.h"

namespace dev {

namespace {
constexpr const char* TAG = "CAP_ENFORCER";
}

CapabilityEnforcer::CapabilityEnforcer(DeviceConfig& config) 
    : config_{config} {}

CapabilityEnforcer::result_type CapabilityEnforcer::init()
{
		auto ceiling = config_.capability_ceiling();
		if (!ceiling) {
				ESP_LOGE(TAG, "Failed to load capability ceiling");
				return std::unexpected(EnforcementError::ManifestCorrupt);
		}

		ceiling_mask_ = *ceiling;

		auto active = config_.active_capabilities();
		if (active) {
				// Load persisted value
				active_mask_ = *active;
		} else {
				// No persisted state yet.
				// Normal on first use after enrollment
				active_mask_ = ceiling_mask_;
				auto stored = config_.store_active_capabilities(active_mask_);
				if (!stored) {
						ESP_LOGE(TAG, "Failed to persist initial active capability state");
						return std::unexpected(EnforcementError::ManifestCorrupt);
				}
		}

		// Quarantined is a state where a device with some capability ceiling
		// but no active capabilities.
		quarantined_ = (active_mask_ == 0 && ceiling_mask_ != 0);

		initialized_ = true;
		ESP_LOGI(TAG, "Initialized: ceiling=0x%08" PRIx32 " active=0x%08" PRIx32
								  " quarantined=%d", ceiling_mask_, active_mask_, quarantined_);

		return {};
}

CapabilityEnforcer::result_type CapabilityEnforcer::check(Capability cap) const
{
		if (!initialized_)
				return std::unexpected(EnforcementError::NotInitialized);
		if (quarantined_)
				return std::unexpected(EnforcementError::DeviceQuarantined);

		uint32_t bit = static_cast<uint32_t>(cap);
		if (!(ceiling_mask_ & bit))
				return std::unexpected(EnforcementError::CapabilityDenied);
		if (!(active_mask_ & bit))
				return std::unexpected(EnforcementError::DeviceRestricted);

    return {};
}

CapabilityEnforcer::result_type 
CapabilityEnforcer::apply_restriction(uint32_t permitted_mask)
{
		if (!initialized_)
				return std::unexpected(EnforcementError::NotInitialized);

    // Invariant ActiveSubset
    const uint32_t bounded = permitted_mask & ceiling_mask_;
    if (bounded != permitted_mask) {
        // Gateway sent a mask exceeding ceiling.
        // Apply the intersection silently and log warning.
        // TODO: Consider letting the gateway know.
        ESP_LOGW(TAG, "Restrition mask 0x%08" PRIx32 " exceeds ceiling 0x%08" PRIx32 
											", intersection 0x%08" PRIx32 " applied", 
											permitted_mask, ceiling_mask_, bounded);
    }

		auto stored = config_.store_active_capabilities(bounded);
		if (!stored) {
				ESP_LOGE(TAG, "Failed to persist restriction");
				return std::unexpected(EnforcementError::ManifestCorrupt);
		}

    active_mask_ = bounded;
		quarantined_ = (active_mask_ == 0 && ceiling_mask_ != 0);
    return {};
}

CapabilityEnforcer::result_type CapabilityEnforcer::quarantine()
{
		if (!initialized_)
				return std::unexpected(EnforcementError::NotInitialized);

		auto stored = config_.store_active_capabilities(0);
		if (!stored) {
				ESP_LOGE(TAG, "Failed to persist quarantine");
				return std::unexpected(EnforcementError::ManifestCorrupt);
		}

    active_mask_ = 0;
		quarantined_ = true;
    ESP_LOGW(TAG, "Device quarantined");
    return {};
}

CapabilityEnforcer::result_type CapabilityEnforcer::restore_to_ceiling()
{
		if (!initialized_)
				return std::unexpected(EnforcementError::NotInitialized);

		auto stored = config_.store_active_capabilities(ceiling_mask_);
		if (!stored) {
				ESP_LOGE(TAG, "Failed to persist restoration");
				return std::unexpected(EnforcementError::ManifestCorrupt);
		}

    active_mask_ = ceiling_mask_;
		quarantined_ = false;
    ESP_LOGI(TAG, "Restored to full ceiling: 0x%08" PRIx32, ceiling_mask_);
    return {};
}

uint32_t CapabilityEnforcer::active_capabilities() const
{
    return active_mask_;
}

uint32_t CapabilityEnforcer::manifest_capabilities() const
{
    return ceiling_mask_;
}

bool CapabilityEnforcer::is_qurantined() const
{
		return quarantined_;
}

} // namespace dev
