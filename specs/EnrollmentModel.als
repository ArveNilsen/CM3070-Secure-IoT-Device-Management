sig Device {}
sig Capability {}
sig HardwareIdentity {}

sig Manifest {
    c: set Capability,
    hi: one HardwareIdentity
}

-- Attestation evidence produced by a device before enrollment.
sig AttestationEvidence {
	claimed: one HardwareIdentity
}

-- A device class determines the maximum capability set
-- permitted at enrollment.
abstract sig DeviceClass {
	ceiling: set Capability
}
one sig Sensor, Actuator, Controller extends DeviceClass {}

-- System state before and after enrollment
sig SystemState {
	enrolled: set Device,
	manifests: Device -> lone Manifest,
	deviceClass: Device -> one DeviceClass,
	activeCapabilities: Device -> set Capability
}

pred validPreState[s: SystemState, d: Device, e: AttestationEvidence] {
	-- Device is not already enrolled
	d not in s.enrolled

	-- No existing manifest is bound to this hardware identity
	all m: Manifest |
		m in s.manifests[Device] implies
		m.hi != e.claimed

	-- Attestation evidence references a known hardware identity
	some h: HardwareIdentity | h = e.claimed
}

-- Postcondition for successful enrollment
pred enrollSuccess[pre, post: SystemState,
					d: Device,
					e: AttestationEvidence,
					m: Manifest] {
	-- Precondition must hold
	validPreState[pre, d, e]

	-- Manifest is bound to the attested hardware identity
	m.hi = e.claimed

	-- Manifest capability set is within the ceiling for the device's class
	m.c in pre.deviceClass[d].ceiling

	-- Post state: device is enrolled
	post.enrolled = pre.enrolled + d

	-- Post state: manifest is assigned
	post.manifests = pre.manifests ++ (d -> m)

	-- Post state: active capabilities init to full manifest
	post.activeCapabilities =
		pre.activeCapabilities ++ (d -> m.c)

	-- Post state: device class unchanged
	post.deviceClass = pre.deviceClass

	-- No other device's manifest or capabilities change
	all other: Device - d | {
		post.manifests[other] = pre.manifests[other]
		post.activeCapabilities[other] =
			pre.activeCapabilities[other]
	}
}

pred enrollFail[pre, post: SystemState,
			d: Device,
			e: AttestationEvidence] {
	-- Precondition initiated but attestation cannot be verified
	d not in pre.enrolled

	-- Post state: device remains unenrolled
	post.enrolled = pre.enrolled

	-- Post state: no manifest assigned
	post.manifests = pre.manifests

	-- Post state: no capabilities granted
	post.activeCapabilities = pre.activeCapabilities

	-- Nothing changes
	post.deviceClass = pre.deviceClass
}

-- Only enrolled deviced have manifests
fact ManifestOnlyIfEnrolled {
	all s: SystemState, d: Device |
		d not in s.enrolled implies
		no s.manifests[d]
}

-- Only enrolled devices have active capabilities
fact CapabilitiesOnlyIfEnrolled {
	all s: SystemState, d: Device |
		d not in s.enrolled implies
		no s.activeCapabilities[d]
}

-- Active capabilities never exceed manifest
fact ActiveSubsetOfManifest {
	all s: SystemState, d: Device |
	d in s.enrolled implies
	s.activeCapabilities[d] in s.manifests[d].c
}

-- Manifest capabilities never exceed device class ceiling
fact ManifestWithinCeiling {
	all s: SystemState, d: Device |
	d in s.enrolled implies
	s.manifests[d].c in s.deviceClass[d].ceiling
}

-- No two enrolled deviced share a manifest
fact UniqueManifestPerDevice {
	all s: SystemState, d1, d2: Device |
	(d1 in s.enrolled and d2 in s.enrolled
	and s.manifests[d1] = s.manifests[d2])
	implies d1 = d2
}

-- No two enrolled devices share a hardware identity
fact UniqueHardwareIdentity {
	all s: SystemState, d1, d2: Device |
	(d1 in s.enrolled and d2 in s.enrolled
	and s.manifests[d1].hi = s.manifests[d2].hi)
	implies d1 = d2
}

-- Enrollment never grants capabilities exeeding the class ceiling
assert EnrollmentRespectsClassCeiling {
	all pre, post: SystemState,
		d: Device, e: AttestationEvidence,
		m: Manifest |
		enrollSuccess[pre, post, d, e, m] implies
		post.activeCapabilities[d] in 
			post.deviceClass[d].ceiling
}
check EnrollmentRespectsClassCeiling for 3

-- A device cannot be enrolled twice without an intervening revocation
assert NoDoubleEnrollment {
	all pre, post: SystemState,
		d: Device, e: AttestationEvidence,
		m: Manifest |
		enrollSuccess[pre, post, d, e, m] implies
		d not in pre.enrolled
}
check NoDoubleEnrollment for 3

-- Failed enrollment grants no capabilities
assert FailedEnrollmentGrantsNothing {
	all pre, post: SystemState,
	d: Device, e: AttestationEvidence |
	enrollFail[pre, post, d, e] implies
	no post.activeCapabilities[d]
}
check FailedEnrollmentGrantsNothing for 3

-- Enrollment does not affect other devices
assert EnrollmentIsLocal {
	all pre, post: SystemState,
	d: Device, e: AttestationEvidence,
	m: Manifest |
	enrollSuccess[pre, post, d, e, m] implies
	all other: Device - d |
		post.activeCapabilities[other] =
			pre.activeCapabilities[other]
}
check EnrollmentIsLocal for 3

-- Hardware identity uniqueness is preserved across enrollment
assert HardwareIdentityPreserved {
	all pre, post: SystemState,
		d: Device, e: AttestationEvidence,
		m: Manifest |
		enrollSuccess[pre, post, d, e, m] implies
		all other: Device |
			(other in post.enrolled and other != d)
			implies post.manifests[other].hi != m.hi
}
check HardwareIdentityPreserved for 3

-- Confirm a successful enrollment is possible
run enrollSuccess for 3 SystemState, 3 Device, 3 Manifest, 
	4 Capability, 3 HardwareIdentity, 3 AttestationEvidence

-- Confirm a failed enrollment is possible
run enrollFail for 2 SystemState, 2 Device, 2 Manifest, 
	3 Capability, 2 HardwareIdentity, 2 AttestationEvidence

-- Confirm a system with mixed enrollment is possible
pred mixedState {
	some s: SystemState |
		some s.enrolled and 
		some (Device - s.enrolled)
}
run mixedState for 3
