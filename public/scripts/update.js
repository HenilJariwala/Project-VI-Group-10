const $ = (id) => document.getElementById(id);

const flightSelect = $("flightSelect");
const reloadBtn = $("reloadBtn");
const resetBtn = $("resetBtn");
const form = $("updateFlightForm");
const msg = $("msg");

const planeID = $("planeID");
const originAirportID = $("originAirportID");
const destinationAirportID = $("destinationAirportID");
const airlineID = $("airlineID");
const gate = $("gate");
const passengerCount = $("passengerCount");
const departureTime = $("departureTime");

let lastLoadedFlight = null;

function setMsg(text, isError = false) {
  msg.textContent = text || "";
  msg.classList.toggle("error", !!isError);
}

async function fetchJson(url, opts) {
  const res = await fetch(url, opts);
  const ct = res.headers.get("content-type") || "";
  const isJson = ct.includes("application/json");
  const body = isJson ? await res.json() : await res.text();

  if (!res.ok) {
    const err = typeof body === "string" ? body : (body?.message || JSON.stringify(body));
    throw new Error(err || `HTTP ${res.status}`);
  }

  return body;
}

// Convert ISO -> datetime-local in *local time*
function isoToLocalDatetimeInput(iso) {
  if (!iso) return "";
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return "";

  const pad = (n) => String(n).padStart(2, "0");
  const yyyy = d.getFullYear();
  const mm = pad(d.getMonth() + 1);
  const dd = pad(d.getDate());
  const hh = pad(d.getHours());
  const min = pad(d.getMinutes());
  return `${yyyy}-${mm}-${dd}T${hh}:${min}`;
}

// Convert datetime-local -> ISO string (UTC)
function localDatetimeInputToIso(dtLocal) {
  if (!dtLocal) return "";
  const d = new Date(dtLocal); // interpreted as local
  if (Number.isNaN(d.getTime())) return "";
  return d.toISOString();
}

function selectedPlaneMaxSeats() {
  const opt = planeID.selectedOptions?.[0];
  if (!opt) return null;
  const v = Number(opt.dataset.maxSeats);
  return Number.isFinite(v) ? v : null;
}

async function loadPlanes() {
  planeID.innerHTML = "";
  const data = await fetchJson("/api/planes");
  const planes = data.planes || [];

  planes.forEach((p) => {
    const opt = document.createElement("option");
    opt.value = p.planeID;
    opt.textContent = `${p.model} (seats ${p.maxSeats})`;
    opt.dataset.maxSeats = p.maxSeats;
    planeID.appendChild(opt);
  });
}

async function loadAirports() {
  originAirportID.innerHTML = "";
  destinationAirportID.innerHTML = "";

  const data = await fetchJson("/api/airports");
  const airports = data.airports || [];

  airports.forEach((a) => {
    const label = `${a.code} - ${a.city}`;

    const opt1 = document.createElement("option");
    opt1.value = a.airportID;
    opt1.textContent = label;
    originAirportID.appendChild(opt1);

    const opt2 = document.createElement("option");
    opt2.value = a.airportID;
    opt2.textContent = label;
    destinationAirportID.appendChild(opt2);
  });
}

async function loadAirlines() {
  airlineID.innerHTML = "";
  const data = await fetchJson("/api/airlines");
  const airlines = data.airlines || [];

  airlines.forEach((a) => {
    const opt = document.createElement("option");
    opt.value = a.airlineID;
    opt.textContent = a.name;
    airlineID.appendChild(opt);
  });
}

async function loadFlightList(selectFlightId = null) {
  flightSelect.innerHTML = "";

  const data = await fetchJson("/api/flights");
  const flights = data.flights || [];

  if (!flights.length) {
    const opt = document.createElement("option");
    opt.value = "";
    opt.textContent = "No flights found";
    flightSelect.appendChild(opt);
    flightSelect.disabled = true;
    return;
  }

  flightSelect.disabled = false;

  flights.forEach((f) => {
    const opt = document.createElement("option");
    opt.value = f.flightID;

    const when = (f.departureTime || "").replace("T", " ").slice(0, 16);
    const airlineName = f.airline?.name ?? "Unknown Airline";
    const o = f.origin?.code ?? "???";
    const d = f.destination?.code ?? "???";

    opt.textContent = `#${f.flightID} • ${airlineName} • ${o} → ${d} • ${when}`;
    flightSelect.appendChild(opt);
  });

  if (selectFlightId) {
    flightSelect.value = String(selectFlightId);
  }
}

async function loadFlightIntoForm(flightIDValue) {
  setMsg("");
  if (!flightIDValue) return;

  const f = await fetchJson(`/api/flights/${flightIDValue}`);
  lastLoadedFlight = f;

  planeID.value = String(f.planeID);
  originAirportID.value = String(f.originAirportID);
  destinationAirportID.value = String(f.destinationAirportID);
  airlineID.value = String(f.airlineID);

  gate.value = f.gate || "";
  passengerCount.value = (f.passengerCount ?? "") + "";
  departureTime.value = isoToLocalDatetimeInput(f.departureTime);
}

function readFormPayload() {
  return {
    planeID: Number(planeID.value),
    originAirportID: Number(originAirportID.value),
    destinationAirportID: Number(destinationAirportID.value),
    airlineID: Number(airlineID.value),
    gate: gate.value.trim(),
    passengerCount: Number(passengerCount.value),
    departureTime: localDatetimeInputToIso(departureTime.value),
  };
}

function validatePayload(payload) {
  if (!payload.planeID || !payload.airlineID || !payload.originAirportID || !payload.destinationAirportID) {
    throw new Error("Please fill out all dropdowns.");
  }

  if (payload.originAirportID === payload.destinationAirportID) {
    throw new Error("Origin and destination must be different.");
  }

  if (!payload.gate) {
    throw new Error("Gate is required.");
  }

  if (!Number.isFinite(payload.passengerCount) || payload.passengerCount < 0) {
    throw new Error("Passenger count must be 0 or more.");
  }

  const maxSeats = selectedPlaneMaxSeats();
  if (maxSeats != null && payload.passengerCount > maxSeats) {
    throw new Error(`Passenger count cannot exceed max seats (${maxSeats}).`);
  }

  if (!payload.departureTime) {
    throw new Error("Departure time is required.");
  }
}

async function boot() {
  // Load reference data first so selects are populated before we set .value
  await Promise.all([loadPlanes(), loadAirports(), loadAirlines()]);
  await loadFlightList();

  const first = flightSelect.value;
  if (first) await loadFlightIntoForm(first);
}

reloadBtn.addEventListener("click", async () => {
  try {
    setMsg("Refreshing...");
    const current = flightSelect.value;
    await loadFlightList(current);
    if (flightSelect.value) await loadFlightIntoForm(flightSelect.value);
    setMsg("List refreshed.");
  } catch (e) {
    setMsg(e.message, true);
  }
});

flightSelect.addEventListener("change", async () => {
  try {
    await loadFlightIntoForm(flightSelect.value);
    setMsg("");
  } catch (e) {
    setMsg(e.message, true);
  }
});

resetBtn.addEventListener("click", () => {
  if (!lastLoadedFlight) return;

  planeID.value = String(lastLoadedFlight.planeID);
  originAirportID.value = String(lastLoadedFlight.originAirportID);
  destinationAirportID.value = String(lastLoadedFlight.destinationAirportID);
  airlineID.value = String(lastLoadedFlight.airlineID);

  gate.value = lastLoadedFlight.gate || "";
  passengerCount.value = (lastLoadedFlight.passengerCount ?? "") + "";
  departureTime.value = isoToLocalDatetimeInput(lastLoadedFlight.departureTime);

  setMsg("Reset to last loaded values.");
});

form.addEventListener("submit", async (e) => {
  e.preventDefault();

  try {
    const id = flightSelect.value;
    if (!id) throw new Error("Choose a flight first.");

    const payload = readFormPayload();
    validatePayload(payload);

    setMsg("Saving...");

    await fetchJson(`/api/flights/${id}`, {
      method: "PUT",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });

    await loadFlightIntoForm(id);
    await loadFlightList(id); // keep list text in sync (if airline/time changed)
    setMsg(`Flight #${id} updated.`);
  } catch (e) {
    setMsg(e.message, true);
  }
});

boot().catch((e) => setMsg(e.message, true));
