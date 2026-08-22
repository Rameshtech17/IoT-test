import os
from datetime import datetime, timezone

from fastapi import FastAPI, HTTPException, Header
from pydantic import BaseModel
from supabase import create_client, Client
from fastapi.middleware.cors import CORSMiddleware


app = FastAPI(
    title="ESP8266 IoT API",
    version="1.0.0"
)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*","http://localhost:5173/"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)

# --------------------------------------------------
# Supabase
# --------------------------------------------------

SUPABASE_URL = os.getenv("SUPABASE_URL")
SUPABASE_KEY = os.getenv("SUPABASE_KEY")
DEVICE_API_KEY = os.getenv("DEVICE_API_KEY")

if not SUPABASE_URL or not SUPABASE_KEY:
    raise RuntimeError("Supabase environment variables are missing")

supabase: Client = create_client(
    SUPABASE_URL,
    SUPABASE_KEY
)


# --------------------------------------------------
# Models
# --------------------------------------------------

class SensorData(BaseModel):
    device_id: str
    temperature: float
    humidity: float


# --------------------------------------------------
# Root
# --------------------------------------------------

@app.get("/")
def root():
    return {
        "success": True,
        "message": "ESP8266 IoT API is running"
    }


# --------------------------------------------------
# Health
# --------------------------------------------------

@app.get("/health")
def health():
    return {
        "status": "healthy"
    }


# --------------------------------------------------
# Receive sensor data
# --------------------------------------------------

@app.post("/api/sensors")
def receive_sensor_data(
    data: SensorData,
    x_api_key: str | None = Header(default=None)
):

    # Check API key
    if x_api_key != DEVICE_API_KEY:
        raise HTTPException(
            status_code=401,
            detail="Invalid API key"
        )

    # Validate values
    if data.humidity < 0 or data.humidity > 100:
        raise HTTPException(
            status_code=400,
            detail="Invalid humidity"
        )

    if data.temperature < -50 or data.temperature > 100:
        raise HTTPException(
            status_code=400,
            detail="Invalid temperature"
        )

    record = {
        "device_id": data.device_id,
        "temperature": data.temperature,
        "humidity": data.humidity,
        "created_at": datetime.now(timezone.utc).isoformat()
    }

    try:
        response = (
            supabase
            .table("sensor_readings")
            .insert(record)
            .execute()
        )

        return {
            "success": True,
            "message": "Sensor data saved",
            "data": response.data
        }

    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail=str(e)
        )


# --------------------------------------------------
# Get latest reading
# --------------------------------------------------

@app.get("/api/sensors/{device_id}/latest")
def latest_sensor_data(device_id: str):

    try:
        response = (
            supabase
            .table("sensor_readings")
            .select("*")
            .eq("device_id", device_id)
            .order("created_at", desc=True)
            .limit(1)
            .execute()
        )

        if not response.data:
            raise HTTPException(
                status_code=404,
                detail="No sensor data found"
            )

        return {
            "success": True,
            "data": response.data[0]
        }

    except HTTPException:
        raise

    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail=str(e)
        )


# --------------------------------------------------
# Get sensor history
# --------------------------------------------------

@app.get("/api/sensors/{device_id}")
def sensor_history(
    device_id: str,
    limit: int = 100
):

    if limit > 1000:
        limit = 1000

    try:
        response = (
            supabase
            .table("sensor_readings")
            .select("*")
            .eq("device_id", device_id)
            .order("created_at", desc=True)
            .limit(limit)
            .execute()
        )

        return {
            "success": True,
            "device_id": device_id,
            "count": len(response.data),
            "data": response.data
        }

    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail=str(e)
        )
