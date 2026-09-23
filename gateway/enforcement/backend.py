from abc import ABC, abstractmethod

class NetworkEnforcementBackend(ABC):
    @abstractmethod
    def quarantine(self, mac_address: str) -> None: ...
    @abstractmethod
    def restore(self, mac_address: str) -> None: ...

class NullBackend(NetworkEnforcementBackend):
    def quarantine(self, mac_address: str) -> None:
        print(f"[Nullbackend] would quarantine {mac_address}")
    def restore(self, mac_address: str) -> None:
        print(f"[Nullbackend] would restore {mac_address}")
