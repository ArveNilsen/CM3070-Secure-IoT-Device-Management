#pragma once

#include <string_view>
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
    CapabilityDenied,
    DeviceNotEnrolled,
    ManifestCorrupt,
    DeviceQuarantined,
    DeviceRestricted
};

class CapabilityEnforcer {
    explicit CapabilityEnforcer(DeviceConfig& config);

    /**
     * @brief Check if capability is currently permitted.
     * Called before every outbound action.
     */
    std::expected<void, EnforcementError> check(Capability cap) const;

    /**
     * @brief Apply restriction received from gateway, 
     * reduces active set below ceiling.
     * Cannot exceed enrolled manifest.
     */
    std::expected<void, EnforcementError>
        apply_restriction(uint32_t permitted_mask);

    /**
     * @brief Apply quarantine. Sets active to empty set.
     */
    void quarantine();

    /**
     * @brief Restore active capabilities to full manifest ceiling.
     * Controlled from gateway command.
     */
    std::expected<void, EnforcementError> restore_to_ceiling();

    /**
     * @brief Current active capability mask
     */
    uint32_t active_capabilities() const;

    /**
     * @brief Enrolled manifest ceiling
     */
    uint32_t manifest_capabilities() const;

private:
    DeviceConfig&   config_;
    uint32_t        active_mask_    = 0;
    uint32_t        ceiling_mask_   = 0;
    bool            quarantined_    = false;

    /**
     * @brief Load ceiling from stored manifest, called at init
     */
    std::expected<void, EnforcementError> load_manifest();
};

} // namespace dev
