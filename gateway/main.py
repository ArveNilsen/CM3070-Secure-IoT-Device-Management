import uvicorn
from fastapi import FastAPI

import enrollment.router as enrollment_module
from enrollment.router import router as enrollment_router
from registry.store import DeviceRegistry

app = FastAPI(
    title="IoT Enrollment Gateway",
    description="Capability-bounded IIot device enrollment service")

# Single registry
registry = DeviceRegistry("registry.db")

# Inject registry into enrollment router
enrollment_module.registry = registry

app.include_router(enrollment_router)

@app.get("/health")
async def health():
    return {"status": "ok", "stub_mode": True}

if __name__ == "__main__":
    uvicorn.run(app,
                host="0.0.0.0",
                port=8080,
                log_level="info")
