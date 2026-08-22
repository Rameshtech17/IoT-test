import { useEffect, useState } from "react";
import "./App.css";

const API_URL = "https://io-t-test-ckfj.vercel.app";

const DEVICE_ID = "ESP8266_01";
const CONTROL_DEVICE_ID = "ESP8266_02";

function App() {
  const [latest, setLatest] = useState(null);
  const [history, setHistory] = useState([]);
  const [loading, setLoading] = useState(true);
  const [status, setStatus] = useState(false);
  const [error, setError] = useState("");
  const [toggling, setToggling] = useState(false);

  // ==============================
  // LOAD SENSOR + DEVICE DATA
  // ==============================
  async function loadData() {
    try {
      setError("");

      // ------------------------------
      // Get latest sensor data
      // ------------------------------
      const latestResponse = await fetch(
        `${API_URL}/api/sensors/${DEVICE_ID}/latest`
      );

      if (!latestResponse.ok) {
        throw new Error("Failed to get latest sensor data");
      }

      const latestJson = await latestResponse.json();

      console.log("Latest sensor:", latestJson);

      // ------------------------------
      // Get device status
      // ------------------------------
      const latestDeviceResponse = await fetch(
        `${API_URL}/api/device/${CONTROL_DEVICE_ID}/latest`
      );

      if (!latestDeviceResponse.ok) {
        throw new Error("Failed to get latest device data");
      }

      const deviceStatusJson =
        await latestDeviceResponse.json();

      console.log(
        "Latest device response:",
        deviceStatusJson
      );

      // Backend returns true / false
      const backendStatus =
        deviceStatusJson?.data?.status;

      console.log(
        "Backend status:",
        backendStatus,
        typeof backendStatus
      );

      setStatus(backendStatus === true);

      // ------------------------------
      // Get sensor history
      // ------------------------------
      const historyResponse = await fetch(
        `${API_URL}/api/sensors/${DEVICE_ID}?limit=20`
      );

      if (!historyResponse.ok) {
        throw new Error("Failed to get sensor history");
      }

      const historyJson =
        await historyResponse.json();

      console.log(
        "Sensor history:",
        historyJson
      );

      setLatest(latestJson.data);
      setHistory(historyJson.data || []);
    } catch (err) {
      console.error("Load data error:", err);

      setError(
        err.message || "Something went wrong"
      );
    } finally {
      setLoading(false);
    }
  }

  // ==============================
  // TOGGLE DEVICE
  // ==============================
  const toggleDevice = async () => {
    if (toggling) {
      return;
    }

    // Calculate the NEW status
    const newStatus = !status;

    console.log("Current status:", status);
    console.log("New status:", newStatus);

    // Update UI immediately
    setStatus(newStatus);

    setToggling(true);
    setError("");

    try {
      const response = await fetch(
        `${API_URL}/api/devicecontrol`,
        {
          method: "POST",

          headers: {
            accept: "application/json",
            "x-api-key":
              "ESP8266_DHT11_2026_8f72c91",
            "Content-Type":
              "application/json",
          },

          body: JSON.stringify({
            device_id: CONTROL_DEVICE_ID,
            status: newStatus,
          }),
        }
      );

      if (!response.ok) {
        throw new Error(
          `Device control failed. HTTP status: ${response.status}`
        );
      }

      const data = await response.json();

      console.log(
        "Device updated successfully:",
        data
      );

      // Get actual status from backend
      await loadData();
    } catch (err) {
      console.error(
        "Failed to control device:",
        err
      );

      // Revert UI if API failed
      setStatus(status);

      setError(
        err.message ||
          "Failed to control device"
      );
    } finally {
      setToggling(false);
    }
  };

  // ==============================
  // INITIAL LOAD
  // ==============================
  useEffect(() => {
    loadData();

    // Refresh every 10 seconds
    const interval = setInterval(() => {
      loadData();
    }, 10000);

    return () => {
      clearInterval(interval);
    };
  }, []);

  // ==============================
  // LOADING
  // ==============================
  if (loading) {
    return (
      <div className="app">
        <h1>ESP8266 IoT Dashboard</h1>

        <div className="loading">
          Loading sensor data...
        </div>
      </div>
    );
  }

  // ==============================
  // UI
  // ==============================
  return (
    <div className="app">
      {/* ==========================
          HEADER
      ========================== */}
      <header>
        <div>
          <h1>ESP8266 IoT Dashboard</h1>

          <p>
            Sensor Device: {DEVICE_ID}
          </p>

          <p>
            Control Device: {CONTROL_DEVICE_ID}
          </p>
        </div>

        <div className="header-buttons">
          {/* Refresh */}
          <button
            onClick={loadData}
            disabled={toggling}
          >
            Refresh
          </button>

          {/* ON / OFF */}
          <button
            onClick={toggleDevice}
            disabled={toggling}
            className={
              status
                ? "device-on"
                : "device-off"
            }
          >
            {toggling
              ? "Updating..."
              : status
              ? "ON"
              : "OFF"}
          </button>
        </div>
      </header>

      {/* ==========================
          ERROR
      ========================== */}
      {error && (
        <div className="error">
          {error}
        </div>
      )}

      {/* ==========================
          SENSOR DATA
      ========================== */}
      {latest && (
        <>
          <div className="cards">
            {/* Temperature */}
            <div className="card temperature">
              <div className="icon">
                🌡️
              </div>

              <div>
                <p>Temperature</p>

                <h2>
                  {latest.temperature} °C
                </h2>
              </div>
            </div>

            {/* Humidity */}
            <div className="card humidity">
              <div className="icon">
                💧
              </div>

              <div>
                <p>Humidity</p>

                <h2>
                  {latest.humidity} %
                </h2>
              </div>
            </div>

            {/* Device */}
            <div className="card device">
              <div className="icon">
                📡
              </div>

              <div>
                <p>Device</p>

                <h2>
                  {latest.device_id}
                </h2>
              </div>
            </div>

            {/* Device Status */}
            <div className="card device-status">
              <div className="icon">
                {status ?   "🔴":"🟢"}
              </div>

              <div>
                <p>Device Status</p>

                <h2>
                  {status ?  "OFF":"ON" }
                </h2>
              </div>
            </div>
          </div>

          {/* Last update */}
          <div className="last-update">
            Last update:{" "}
            {latest.created_at
              ? new Date(
                  latest.created_at
                ).toLocaleString()
              : "N/A"}
          </div>
        </>
      )}

      {/* ==========================
          HISTORY
      ========================== */}
      <section className="history">
        <div className="section-header">
          <h2>Sensor History</h2>

          <span>
            {history.length} readings
          </span>
        </div>

        <div className="table-container">
          <table>
            <thead>
              <tr>
                <th>Time</th>
                <th>Device</th>
                <th>Temperature</th>
                <th>Humidity</th>
              </tr>
            </thead>

            <tbody>
              {history.length > 0 ? (
                history.map(
                  (reading, index) => (
                    <tr
                      key={
                        reading.id ||
                        reading.created_at ||
                        index
                      }
                    >
                      <td>
                        {reading.created_at
                          ? new Date(
                              reading.created_at
                            ).toLocaleString()
                          : "N/A"}
                      </td>

                      <td>
                        {reading.device_id}
                      </td>

                      <td>
                        {reading.temperature} °C
                      </td>

                      <td>
                        {reading.humidity} %
                      </td>
                    </tr>
                  )
                )
              ) : (
                <tr>
                  <td
                    colSpan="4"
                    style={{
                      textAlign: "center",
                    }}
                  >
                    No sensor readings found
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </div>
      </section>
    </div>
  );
}

export default App;