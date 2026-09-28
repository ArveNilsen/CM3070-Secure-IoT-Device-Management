---------------------- MODULE EnrollmentCapabilitySpec ----------------------
(****************************************************************************
* Formal specification of the device enrollment and capability-bounding 
* protocol.
*
* Core claim verified by this specification (CapabilityCeiling):
*   A device's active capabilities never exceed the capability manifest
*   assigned to it at enrollment, at any point in its operational lifetime.
****************************************************************************)

EXTENDS Naturals, FiniteSets, TLC

CONSTANTS
    Devices,        \* set of device identifiers
    Capabilities,   \* set of all possible capabilities
    Classes,        \* The set of device classes
    DeviceClass,    \* A function Devices -> Classes
    ClassCeiling,   \* A function Classes -> SUBSET Capabilities
    Nonces,         \* The set of possible nonce values
    NoManifest,     \* Sentinel: "no manifest assigned"
    NoNonce         \* Sentinal: "no nonce pending"

(****************************************************************************
* Assumptions about the shape of the constants above.
****************************************************************************)
ASSUME DeviceClass \in [Devices -> Classes]
ASSUME ClassCeiling \in [Classes -> SUBSET Capabilities]
ASSUME \A s \in SUBSET Capabilities : s # NoManifest
\*ASSUME NoManifest \notin SUBSET Capabilities
ASSUME NoNonce \notin Nonces

\*---------------------------------------------------------------------------
\* Helper operators
\*---------------------------------------------------------------------------

\* Strict superset: A is a strict superset of B
\* iff B is a subset of A and A is not equal to B
StrictSuperset(A, B) == B \subseteq A /\ A # B

\* The set of lifecycle states a device may occupy.
States == {"Unenrolled", "Attesting", "Enrolled", "Restricted", "Quarantined", "Revoked"}

\*---------------------------------------------------------------------------
\* Variables
\*---------------------------------------------------------------------------

VARIABLES
    deviceState,        \* Device -> {"Unenrolled", "Attesting", "Enrolled", "Restricted", "Quarantined", "Revoked"}
    enrolledManifest,   \* Device -> SUBSET Capabilities \union {NoManifest}
    activeCapabilities, \* Device -> SUBSET Capabilities
    pendingNonce        \* Device -> Nonces \union {NoNonce}

vars == <<deviceState, enrolledManifest, activeCapabilities, pendingNonce>>

TypeInvariant ==
    /\ deviceState \in [Devices -> States]
    /\ enrolledManifest \in [Devices -> SUBSET Capabilities \union {NoManifest}]
    /\ activeCapabilities \in [Devices -> SUBSET Capabilities]
    /\ pendingNonce \in [Devices -> Nonces \union {NoNonce}]

\*---------------------------------------------------------------------------
\* Initial state: Every device starts unenrolled, with no manifest, no active
\* capabilities, and no pending enrollment.
\*---------------------------------------------------------------------------

Init ==
    /\ deviceState = [d \in Devices |-> "Unenrolled"]
    /\ enrolledManifest = [d \in Devices |-> NoManifest]
    /\ activeCapabilities = [d \in Devices |-> {}]
    /\ pendingNonce = [d \in Devices |-> NoNonce]

\*---------------------------------------------------------------------------
\* Enrollment actions
\*---------------------------------------------------------------------------

\* A device requests a nonce and begins attestation.
InitiateEnrollment(d) ==
    /\ deviceState[d] = "Unenrolled"
    /\ \E n \in Nonces : pendingNonce' = [pendingNonce EXCEPT ![d] = n]
    /\ deviceState' = [deviceState EXCEPT ![d] = "Attesting"]
    /\ UNCHANGED <<enrolledManifest, activeCapabilities>>

\* Attestation succeds, manifest issued at full ceiling for the device's class
EnrollmentSuccess(d) ==
    /\ deviceState[d] = "Attesting"
    /\ pendingNonce[d] # NoNonce
    /\ LET ceiling == ClassCeiling[DeviceClass[d]] IN
        /\ enrolledManifest' = [enrolledManifest EXCEPT ![d] = ceiling]
        /\ activeCapabilities' = [activeCapabilities EXCEPT ![d] = ceiling]
    /\ deviceState' = [deviceState EXCEPT ![d] = "Enrolled"]
    /\ pendingNonce' = [pendingNonce EXCEPT ![d] = NoNonce]

\* Attestation fails, device returns to Unenrolled
EnrollmentFailure(d) ==
    /\ deviceState[d] = "Attesting"
    /\ deviceState' = [deviceState EXCEPT ![d] = "Unenrolled"]
    /\ pendingNonce' = [pendingNonce EXCEPT ![d] = NoNonce]
    /\ UNCHANGED <<enrolledManifest, activeCapabilities>>

\*---------------------------------------------------------------------------
\* Capability enforcement actions
\*---------------------------------------------------------------------------

\* Operator or automated response reduces active capabilities below the ceiling.
Restrict(d, newActive) ==
    /\ deviceState[d] \in {"Enrolled", "Restricted"}
    /\ newActive \subseteq enrolledManifest[d]
    /\ activeCapabilities' = [activeCapabilities EXCEPT ![d] = newActive]
    /\ deviceState' = [deviceState EXCEPT ![d] = "Restricted"]
    /\ UNCHANGED <<enrolledManifest, pendingNonce>>

\* Restore active capabilities to full ceiling, only valid from Restricted, not Quarantined.
Restore(d) ==
    /\ deviceState[d] = "Restricted"
    /\ activeCapabilities' = [activeCapabilities EXCEPT ![d] = enrolledManifest[d]]
    /\ deviceState' = [deviceState EXCEPT ![d] = "Enrolled"]
    /\ UNCHANGED <<enrolledManifest, pendingNonce>>

\* Full isolation, active capabilities emptied
Quarantine(d) ==
    /\ deviceState[d] \in {"Enrolled", "Restricted"}
    /\ activeCapabilities' = [activeCapabilities EXCEPT ![d] = {}]
    /\ deviceState' = [deviceState EXCEPT ![d] = "Quarantined"]
    /\ UNCHANGED <<enrolledManifest, pendingNonce>>

\*---------------------------------------------------------------------------
\* Revocation and re-enrollment
\*---------------------------------------------------------------------------

\* Permanently invalidate enrollment
Revoke(d) ==
    /\ deviceState[d] \in {"Enrolled", "Restricted", "Quarantined"}
    /\ deviceState' = [deviceState EXCEPT ![d] = "Revoked"]
    /\ activeCapabilities' = [activeCapabilities EXCEPT ![d] = {}]
    /\ enrolledManifest' = [enrolledManifest EXCEPT ![d] = NoManifest]
    /\ UNCHANGED pendingNonce

\* Re-enrollment
ReenrollFromRevoked(d) ==
    /\ deviceState[d] = "Revoked"
    /\ deviceState' = [deviceState EXCEPT ![d] = "Unenrolled"]
    /\ UNCHANGED <<enrolledManifest, activeCapabilities, pendingNonce>>

\*---------------------------------------------------------------------------
\* Next-state relation and specification
\*---------------------------------------------------------------------------

Next ==
    \/ \E d \in Devices : InitiateEnrollment(d)
    \/ \E d \in Devices : EnrollmentSuccess(d)
    \/ \E d \in Devices : EnrollmentFailure(d)
    \/ \E d \in Devices, s \in SUBSET Capabilities : Restrict(d, s)
    \/ \E d \in Devices : Restore(d)
    \/ \E d \in Devices : Quarantine(d)
    \/ \E d \in Devices : Revoke(d)
    \/ \E d \in Devices : ReenrollFromRevoked(d)


Spec == Init /\ [][Next]_vars

\* Fairness assumptions: if enrollment can succeed or fail, it eventually will.
\* Used for liveness properties (see below).
Fairness ==
    /\ \A d \in Devices : WF_vars(EnrollmentSuccess(d))
    /\ \A d \in Devices : WF_vars(EnrollmentFailure(d))

SpecWithFairness == Spec /\ Fairness

\*---------------------------------------------------------------------------
\* Safety properties
\*---------------------------------------------------------------------------

\* THE CORE CLAIM: active capabilities never exceed the manifest assigned
\* at enrollment.
CapabilityCeiling ==
    \A d \in Devices :
        enrolledManifest[d] # NoManifest =>
            activeCapabilities[d] \subseteq enrolledManifest[d]

\* No device operates without a valid enrollment
NoCapabilityWithoutEnrollment ==
    \A d \in Devices :
        activeCapabilities[d] # {} =>
            deviceState[d] \in {"Enrolled", "Restricted"}

\* Quarantined devices hold no active capabilities
QuarantineIsIsolated ==
    \A d \in Devices :
        deviceState[d] = "Quarantined" =>
            activeCapabilities[d] = {}

\* Revoked devices hold no manifest
RevokedHasNoManifest ==
 \A d \in Devices :
    deviceState[d] = "Revoked" =>
        enrolledManifest[d] = NoManifest

\* Invariants for every reachable state
THEOREM Spec => []TypeInvariant
THEOREM Spec => []CapabilityCeiling
THEOREM Spec => []NoCapabilityWithoutEnrollment
THEOREM Spec => []QuarantineIsIsolated
THEOREM Spec => []RevokedHasNoManifest

\* No sequence of actions can increase a device's active capabilities beyond
\* what re-enrollment allows. The ceiling is set via EnrollmentSuccess.
NoEscalationBeyondCeiling ==
    [][
        \A d \in Devices :
        StrictSuperset(activeCapabilities'[d],
                          activeCapabilities[d])
        =>
        (enrolledManifest'[d] # enrolledManifest[d]
        \/ activeCapabilities'[d] \subseteq enrolledManifest[d])
    ]_vars

THEOREM Spec => []NoEscalationBeyondCeiling
    
\* A device can only leave the Quarantined state via revocation.
\* There is no direct or indirect path back to Enrolled that does
\* not pass through Revoked (and thus full re-enrollment).
QuarantineRequiresRevocation ==
    [][
        \A d \in Devices :
            deviceState[d] = "Quarantined" /\ deviceState'[d] # "Quarantined"
            => deviceState'[d] = "Revoked"
    ]_vars

THEOREM Spec => []QuarantineRequiresRevocation

\*---------------------------------------------------------------------------
\* Liveness property
\*---------------------------------------------------------------------------
\* A device that begins attesting eventually reaches Enrolled or returns to Unenrolled.
EnrollmentProgress ==
    \A d \in Devices :
        (deviceState[d] = "Attesting") ~>
            (deviceState[d] \in {"Enrolled", "Unenrolled"})

THEOREM SpecWithFairness => EnrollmentProgress
    
=============================================================================
\* Modification History
\* Last modified Fri Sep 25 15:01:20 CEST 2026 by arvenilsen
\* Created Tue Jul 28 13:26:14 CEST 2026 by arvenilsen
