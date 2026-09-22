#pragma once

#include <cstdint>
#include <expected>

namespace dev {

// Capability identifiers
enum class Capability : uint32_t {
    ReadSensorData      = 1 << 0,
    PublishTelemetry    = 1 << 1,
    TriggerActuator     = 1 << 2,
    ReceiveConfig       = 1 << 3,
    BroadcastPresence   = 1 << 4,
};

enum class EnforcementError {
		NotInitialized,
    CapabilityDenied,
    ManifestCorrupt,
    DeviceQuarantined,
    DeviceRestricted
};

class DeviceConfig;

class CapabilityEnforcer {
public:
    using result_type = std::expected<void, EnforcementError>;

    explicit CapabilityEnforcer(DeviceConfig& config);

		/**
		 * @brief Must be called once before other class methods are used.
		 * Loads the enrolled ceiling from the stored manifest.
		 */
		result_type init();

    /**
     * @brief Check if capability is currently permitted.
     * Called before every outbound action.
     */
    result_type check(Capability cap) const;

    /**
     * @brief Apply restriction received from gateway, 
     * reduces active set below ceiling.
     * Cannot exceed enrolled manifest.
     */
    result_type apply_restriction(uint32_t permitted_mask);

    /**
     * @brief Apply quarantine. Sets active to empty set.
     */
    result_type quarantine();

    /**
     * @brief Restore active capabilities to full manifest ceiling.
     * Controlled from gateway command.
     */
    result_type restore_to_ceiling();

    /**
     * @brief Current active capability mask
     */
    uint32_t active_capabilities() const;

    /**
     * @brief Enrolled manifest ceiling
     */
    uint32_t manifest_capabilities() const;

		bool is_qurantined() const;

private:
    DeviceConfig&   config_;
    uint32_t        active_mask_    = 0;
    uint32_t        ceiling_mask_   = 0;
    bool            quarantined_    = false;
		bool						initialized_		= false;
};

} // namespace dev
