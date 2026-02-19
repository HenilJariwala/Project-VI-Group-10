async function fetchOptions(path) {
  const res = await fetch(path, { method: "OPTIONS" });
  const allow = res.headers.get("Allow");
  return {
    status: res.status,
    allow: allow || "(no Allow header returned)"
  };
}

document.addEventListener("DOMContentLoaded", async () => {
  const outFlights = document.getElementById("outFlights");
  const outFlightId = document.getElementById("outFlightId");

  try {
    const flights = await fetchOptions("/api/flights");
    outFlights.textContent =
      `OPTIONS ${"/api/flights"}\nStatus: ${flights.status}\nAllow: ${flights.allow}`;

    const flightId = await fetchOptions("/api/flights/1");
    outFlightId.textContent =
      `OPTIONS ${"/api/flights/1"}\nStatus: ${flightId.status}\nAllow: ${flightId.allow}`;
  } catch (e) {
    outFlights.textContent = "Error calling OPTIONS";
    outFlightId.textContent = "Error calling OPTIONS";
  }
});
