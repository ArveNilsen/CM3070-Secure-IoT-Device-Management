abstract sig DeviceClass {}
one sig Sensor, Actuator, Controller extends DeviceClass {}

sig Device {
	m: one Manifest,
	active: set Capability,
	class: one DeviceClass
}

-- There can be no manifest without hardware binding
sig Manifest {
	c: set Capability,
	hi: one HardwareIdentity
}

sig Capability {}

sig HardwareIdentity {}

-- Active capabilities are a subset of the manifest's capabilities
fact ActiveSubset {
	all d: Device | d.active in d.m.c
}

-- No two devices share the same manifest
fact UniqueManifestPerDevice {
	all d1, d2: Device |
		d1.m = d2.m implies d1 = d2
}

-- No two devices share the same hardware identity
fact UniqueHardwareIdentityPerDevice {
	all d1, d2: Device |
		d1.m.hi = d2.m.hi implies d1 = d2
}

-- Every manifest is owned by exactly one device
fact ManifestOwnership {
	all manifest: Manifest | one d: Device | d.m = manifest
}

-- Every hardware identity is bound to exactly one manifest
fact HardwareIdentityBound {
	all identity: HardwareIdentity | one m: Manifest | m.hi = identity
}

-- A device may have no active capabilities (quarantine/restriction)
pred fullyRestricted[d: Device] {
	no d.active
}
run fullyRestricted for 3

-- Manifest capability is the ceiling
-- Equivalent to ActiveSubset but stated explicilty here for clarity.
assert CapabilityCeiling {
	all d: Device | d.active in d.m.c
}
check CapabilityCeiling for 5

-- No capability exists outside a manifest
fact NoOrphaCapability {
	all cap: Capability | some m: Manifest | cap in m.c
}

-- Manifests are non-transferable
assert ManifestNonTransferable {
	all manifest: Manifest, d1, d2: Device |
		(d1.m = manifest and d2.m = manifest) implies d1 = d2
}
check ManifestNonTransferable for 5

-- Hardware identity determines manifest uniquely
-- Follows from UniqueHardwareIdentityPerDevice and ManifestOwnership
assert HardwareIdentityDeterminesManifest {
	all m1, m2: Manifest |
		m1.hi = m2.hi implies m1 = m2
}
check HardwareIdentityDeterminesManifest for 5

-- Active capability implies manifest membership
assert ActiveImpliesManifestMember {
	all d: Device, cap: Capability |
		cap in d.active implies cap in d.m.c
}
check ActiveImpliesManifestMember for 5

-- Satisfiability check for a fully enrolled system
pred validSystem {
	some d: Device | some d.active
	some d: Device | d.active = d.m.c -- at least one d at full cap
	some d: Device | d.active != d.m.c -- at least one d below cap
}
run validSystem for 4 Device, 4 Manifest, 6 Capability, 4 HardwareIdentity

-- The model supports devices with different sized manifests.
-- Here as a controller with more capabilitites than a sensor
pred classHierarchyConsistent {
	all d1, d2: Device |
		d1.class = Sensor and d2.class = Controller implies
		#d1.m.c <= #d2.m.c
}
run classHierarchyConsistent for 4
