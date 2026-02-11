// loads plane records from GET /api/planes and fills the Plane <select> with readable options
async function loadPlanes() {
  const res = await fetch("/api/planes");
  const data = await res.json();
  const sel = document.getElementById("planeID");

  sel.innerHTML = "";
  data.planes.forEach(p => {
    const opt = document.createElement("option");
    opt.value = p.planeID;
    opt.textContent = `${p.model} (seats ${p.maxSeats})`; // hide speed
    opt.dataset.maxSeats = p.maxSeats;
    sel.appendChild(opt);
  });
}

// loads airport records from GET /api/airports and fills the specified <select> (origin/destination) with options
async function loadAirports(selectId) {
  const res = await fetch("/api/airports");
  const data = await res.json();
  const sel = document.getElementById(selectId);

  sel.innerHTML = "";
  data.airports.forEach(a => {
    const opt = document.createElement("option");
    opt.value = a.airportID;
    opt.textContent = `${a.code} - ${a.city}`;
    sel.appendChild(opt);
  });
}

// airline loader
async function loadAirlines() {
  const res = await fetch("/api/airlines");
  const data = await res.json();
  const sel = document.getElementById("airlineID");

  sel.innerHTML = "";
  data.airlines.forEach(a => {
    const opt = document.createElement("option");
    opt.value = a.airlineID;
    opt.textContent = a.name;
    sel.appendChild(opt);
  });
}

// When the page loads, populate dropdowns from the DB
// and also wire up the form to POST a new flight to /api/flights
document.addEventListener("DOMContentLoaded", async () => {
  await loadPlanes();
  await loadAirlines();
  await loadAirports("originAirportID");
  await loadAirports("destinationAirportID");

  const form = document.getElementById("addFlightForm");
  const msg = document.getElementById("msg");

  form.addEventListener("submit", async (e) => {
    e.preventDefault();
    msg.textContent = "";

    // Read values
    const planeSel = document.getElementById("planeID");
    const planeID = Number(planeSel.value);
    const maxSeats = Number(planeSel.selectedOptions[0].dataset.maxSeats);

    const originAirportID = Number(document.getElementById("originAirportID").value);
    const destinationAirportID = Number(document.getElementById("destinationAirportID").value);

    const passengerCount = Number(document.getElementById("passengerCount").value);

    // Validation: origin != destination
    if (originAirportID === destinationAirportID) {
      msg.textContent = "Origin and destination must be different.";
      return;
    }

    // Validation: passengers <= maxSeats
    if (passengerCount > maxSeats) {
      msg.textContent = `Passenger count cannot exceed max seats (${maxSeats}).`;
      return;
    }

    // Convert datetime-local -> ISO 8601 (UTC)
    const dtLocal = document.getElementById("departureTime").value; // "YYYY-MM-DDTHH:mm"
    if (!dtLocal) {
      msg.textContent = "Departure time is required.";
      return;
    }
    const departureISO = new Date(dtLocal).toISOString();

    msg.textContent = "Submitting...";

    const airlineID = Number(document.getElementById("airlineID").value);
    // Build JSON payload expected by POST /api/flights
    const payload = {
      planeID,
      originAirportID,
      destinationAirportID,
      airlineID,
      gate: document.getElementById("gate").value.trim(),
      passengerCount,
      departureTime: departureISO
    };

    const res = await fetch("/api/flights", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });

    const text = await res.text();
    msg.textContent = `Status ${res.status}: ${text}`;
  });
});




