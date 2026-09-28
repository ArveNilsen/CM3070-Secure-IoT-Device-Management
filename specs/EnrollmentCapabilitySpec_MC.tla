---- MODULE EnrollmentCapabilitySpec_MC ----
(****************************************************************************
* Concrete instantiation of EnrollmentCapabilitySpec, used only for model-
* checking. Defines specific devices, capabilities, and classes so that TLC
* has finite, concrete values to explore.
*
* A reader wishing to understand the protocol should read 
* EnrollmentCapabilitySpec.tla. This module exists purely as mechanical
* configuration for the model checker.
****************************************************************************)

EXTENDS EnrollmentCapabilitySpec

\* Two devices, one each class, is sufficient to execise every action and
\* property in the spec.
MCDeviceClass == 
    "d1" :> "sensor" @@ "d2" :> "actuator"

MCClassCeiling == [
    sensor   |-> {"c1", "c2"},
    actuator |-> {"c3", "c4"}
]

====
