def should_accept(device_state: str, active_caps: int, required_caps: int) -> bool:
    if device_state == "quarantined":
        return False
    return bool(active_caps & required_caps)
