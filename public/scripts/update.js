const $ = (id) => document.getElementById(id);

const flightSelect = $("flightSelect");
const reloadBtn = $("reloadBtn");
const resetBtn = $("resetBtn");
const form = $("updateFlightForm");
const msg = $("msg");

const planeID = $("planeID");
const originAirportID = $("originAirportID");
const destinationAirportID = $("destinationAirportID");
const airline = $("airline");
const gate = $("gate");
const passengerCount = $("passengerCount");
const departureTime = $("departureTime");

let lastLoadedFlight = null;

function setMsg(text, isError = false) {
  msg.textContent = text || "";
  msg.classList.toggle("error", !!isError);
}

function toLocalDatetimeInput(iso) {
  if (!iso) return "";
  return iso.length >= 16 ? iso.slice(0, 16) : iso;
}

function toIsoSecondsFromInput(dtLocal) {
  if (!dtLocal) return "";
  return dtLocal.length === 16 ? `${dtLocal}:00` : dtLocal;
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

async function loadPlanes() {
  planeID.innerHTML = "";
  const data = await fetchJson("/api/planes");
  const planes = data.planes || data;
  (planes || []).forEach((p) => {
    const opt = document.createElement("option");
    opt.value = p.planeID;
    opt.textContent = `${p.model} (max ${p.maxSeats})`;
    planeID.appendChild(opt);
  });
}

async function loadAirports() {
  originAirportID.innerHTML = "";
  destinationAirportID.innerHTML = "";
  const data = await fetchJson("/api/airports");
  const airports = data.airports || data;
  (airports || []).forEach((a) => {
    const label = `${a.code} — ${a.city}`;

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
    opt.textContent = `#${f.flightID} • ${f.airline} • ${f.origin.code} → ${f.destination.code} • ${when}`;
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
  airline.value = f.airline || "";
  gate.value = f.gate || "";
  passengerCount.value = f.passengerCount ?? "";
  departureTime.value = toLocalDatetimeInput(f.departureTime);
}

function readFormPayload() {
  return {
    planeID: Number(planeID.value),
    originAirportID: Number(originAirportID.value),
    destinationAirportID: Number(destinationAirportID.value),
    airline: airline.value.trim(),
    gate: gate.value.trim(),
    passengerCount: Number(passengerCount.value),
    departureTime: toIsoSecondsFromInput(departureTime.value),
  };
}

async function boot() {
  await Promise.all([loadPlanes(), loadAirports()]);
  await loadFlightList();

  const first = flightSelect.value;
  if (first) {
    await loadFlightIntoForm(first);
  }
}

reloadBtn.addEventListener("click", async () => {
  try {
    setMsg("Refreshing...");
    const current = flightSelect.value;
    await loadFlightList(current);
    if (flightSelect.value) {
      await loadFlightIntoForm(flightSelect.value);
    }
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
  if (lastLoadedFlight) {
    planeID.value = String(lastLoadedFlight.planeID);
    originAirportID.value = String(lastLoadedFlight.originAirportID);
    destinationAirportID.value = String(lastLoadedFlight.destinationAirportID);
    airline.value = lastLoadedFlight.airline || "";
    gate.value = lastLoadedFlight.gate || "";
    passengerCount.value = lastLoadedFlight.passengerCount ?? "";
    departureTime.value = toLocalDatetimeInput(lastLoadedFlight.departureTime);
    setMsg("Reset to last loaded values.");
  }
});

form.addEventListener("submit", async (e) => {
  e.preventDefault();
  try {
    const id = flightSelect.value;
    if (!id) throw new Error("Choose a flight first.");

    const payload = readFormPayload();
    if (payload.originAirportID === payload.destinationAirportID) {
      throw new Error("Origin and destination must be different.");
    }

    setMsg("Saving...");
    await fetchJson(`/api/flights/${id}`, {
      method: "PUT",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });

    await loadFlightIntoForm(id);
    setMsg(`Flight #${id} updated.`);
  } catch (e) {
    setMsg(e.message, true);
  }
});

boot().catch((e) => setMsg(e.message, true));
