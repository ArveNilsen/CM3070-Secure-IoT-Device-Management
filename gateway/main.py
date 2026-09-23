import uvicorn
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

import enrollment.router as enrollment_module
from enrollment.router import router as enrollment_router
import management.router as management_module
from management.router import router as management_router
from registry.store import DeviceRegistry
from enforcement.ovs_backend import OVSBackend

app = FastAPI(
    title="IoT Enrollment Gateway",
    description="Capability-bounded IIot device enrollment service")

# CORS middleware - Tighten after testing
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# Single registry
registry = DeviceRegistry("registry.db")

# Inject registry into enrollment router
enrollment_module.registry = registry
management_module.registry = registry

app.include_router(enrollment_router)
app.include_router(management_router)

backend = OVSBackend()
enrollment_module.backend = backend
management_module.backend = backend

@app.get("/health")
async def health():
    return {"status": "ok", "stub_mode": True}

if __name__ == "__main__":
    uvicorn.run(app,
                host="0.0.0.0",
                port=8080,
                log_level="info")
