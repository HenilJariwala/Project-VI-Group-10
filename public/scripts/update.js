const $ = (id) => document.getElementById(id);

const flightSelect = $("flightSelect");
const reloadBtn = $("reloadBtn");
const resetBtn = $("resetBtn");
const form = $("updateFlightForm");
const msg = $("msg");

const checkAllBtn = $("checkAllBtn");
const uncheckAllBtn = $("uncheckAllBtn");

const planeID = $("planeID");
const originAirportID = $("originAirportID");
const destinationAirportID = $("destinationAirportID");
const airlineID = $("airlineID");
const gate = $("gate");
const passengerCount = $("passengerCount");
const departureTime = $("departureTime");

const chk_planeID = $("chk_planeID");
const chk_originAirportID = $("chk_originAirportID");
const chk_destinationAirportID = $("chk_destinationAirportID");
const chk_airlineID = $("chk_airlineID");
const chk_gate = $("chk_gate");
const chk_passengerCount = $("chk_passengerCount");
const chk_departureTime = $("chk_departureTime");

const FIELD_MAP = [
  { cb: chk_planeID, el: planeID, key: "planeID" },
  { cb: chk_originAirportID, el: originAirportID, key: "originAirportID" },
  { cb: chk_destinationAirportID, el: destinationAirportID, key: "destinationAirportID" },
  { cb: chk_airlineID, el: airlineID, key: "airlineID" },
  { cb: chk_gate, el: gate, key: "gate" },
  { cb: chk_passengerCount, el: passengerCount, key: "passengerCount" },
  { cb: chk_departureTime, el: departureTime, key: "departureTime" },
];

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

// ISO -> datetime-local (local time)
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

// datetime-local -> ISO UTC
function localDatetimeInputToIso(dtLocal) {
  if (!dtLocal) return "";
  const d = new Date(dtLocal);
  if (Number.isNaN(d.getTime())) return "";
  return d.toISOString();
}

function selectedPlaneMaxSeats() {
  const opt = planeID.selectedOptions?.[0];
  if (!opt) return null;
  const v = Number(opt.dataset.maxSeats);
  return Number.isFinite(v) ? v : null;
}

function applyCheckboxState() {
  FIELD_MAP.forEach(({ cb, el }) => {
    const enabled = cb.checked;
    el.disabled = !enabled;
    el.required = enabled;
  });
}

function setAllCheckboxes(state) {
  FIELD_MAP.forEach(({ cb }) => {
    cb.checked = state;
  });
  applyCheckboxState();
}

function allFieldsChecked() {
  return FIELD_MAP.every(({ cb }) => cb.checked);
}

FIELD_MAP.forEach(({ cb }) => cb.addEventListener("change", applyCheckboxState));

checkAllBtn.addEventListener("click", () => {
  setAllCheckboxes(true);
  setMsg("All fields selected.");
});

uncheckAllBtn.addEventListener("click", () => {
  setAllCheckboxes(false);
  setMsg("All fields unselected. Select at least one field before saving.");
});

async function loadPlanes() {
  planeID.innerHTML = "";
  const data = await fetchJson("/api/planes");
  const planes = data.planes || [];

  planes.forEach((p) => {
    const opt = document.createElement("option");
    opt.value = p.planeID;
    opt.textContent = `${p.model} (seats ${p.maxSeats})`;
    opt.dataset.maxSeats = String(p.maxSeats);
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

    const o1 = document.createElement("option");
    o1.value = a.airportID;
    o1.textContent = label;
    originAirportID.appendChild(o1);

    const o2 = document.createElement("option");
    o2.value = a.airportID;
    o2.textContent = label;
    destinationAirportID.appendChild(o2);
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

// Fetch all pages (your backend uses ?page= and returns totalPages)
async function fetchAllFlightsPaged() {
  const all = [];

  const first = await fetchJson("/api/flights?page=1");
  all.push(...(first.flights || []));

  const totalPages = Number(first.totalPages) || 1;

  for (let p = 2; p <= totalPages; p++) {
    const data = await fetchJson(`/api/flights?page=${p}`);
    all.push(...(data.flights || []));
  }

  return all;
}

async function loadFlightList(selectFlightId = null) {
  flightSelect.innerHTML = "";
  setMsg("Loading flights...");

  const flights = await fetchAllFlightsPaged();

  if (!flights.length) {
    const opt = document.createElement("option");
    opt.value = "";
    opt.textContent = "No flights found";
    flightSelect.appendChild(opt);
    flightSelect.disabled = true;
    setMsg("No flights found.");
    return;
  }

  flights.sort((a, b) => Number(a.flightID) - Number(b.flightID));

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

  if (selectFlightId) flightSelect.value = String(selectFlightId);
  if (!flightSelect.value) flightSelect.selectedIndex = 0;

  setMsg("");
}

async function loadFlightIntoForm(flightIDValue) {
  setMsg("");
  if (!flightIDValue) return;

  const f = await fetchJson(`/api/flights/${flightIDValue}`);
  lastLoadedFlight = f;

  function setSelect(el, val) {
    const strVal = String(val);
    const match = [...el.options].find(o => o.value === strVal);
    if (match) el.value = strVal;
  }

  setSelect(planeID, f.planeID);
  setSelect(originAirportID, f.originAirportID);
  setSelect(destinationAirportID, f.destinationAirportID);
  setSelect(airlineID, f.airlineID);

  gate.value = f.gate || "";
  passengerCount.value = String(f.passengerCount ?? "");
  departureTime.value = isoToLocalDatetimeInput(f.departureTime);

  setMsg("Flight loaded.");
}

function readFullPayload() {
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

function validateFullPayload(payload) {
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

function buildSelectedPayload(fullPayload) {
  const payload = {};
  FIELD_MAP.forEach(({ cb, key }) => {
    if (cb.checked) payload[key] = fullPayload[key];
  });
  return payload;
}

function validateEffectiveUpdate(selected, original) {
  const effective = {
    planeID: ("planeID" in selected) ? selected.planeID : original.planeID,
    airlineID: ("airlineID" in selected) ? selected.airlineID : original.airlineID,
    originAirportID: ("originAirportID" in selected) ? selected.originAirportID : original.originAirportID,
    destinationAirportID: ("destinationAirportID" in selected) ? selected.destinationAirportID : original.destinationAirportID,
    gate: ("gate" in selected) ? selected.gate : original.gate,
    passengerCount: ("passengerCount" in selected) ? selected.passengerCount : original.passengerCount,
    departureTime: ("departureTime" in selected) ? selected.departureTime : original.departureTime,
  };

  validateFullPayload(effective);
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
  passengerCount.value = String(lastLoadedFlight.passengerCount ?? "");
  departureTime.value = isoToLocalDatetimeInput(lastLoadedFlight.departureTime);

  setMsg("Reset to last loaded values.");
});

form.addEventListener("submit", async (e) => {
  e.preventDefault();

  try {
    const id = flightSelect.value;
    if (!id) throw new Error("Choose a flight first.");
    if (!lastLoadedFlight) throw new Error("Flight details not loaded yet.");

    const fullPayload = readFullPayload();
    const selectedPayload = buildSelectedPayload(fullPayload);

    if (Object.keys(selectedPayload).length === 0) {
      throw new Error("Select at least one field to update.");
    }

    let method;
    let payloadToSend;

    if (allFieldsChecked()) {
      method = "PUT";
      payloadToSend = fullPayload;
      validateFullPayload(fullPayload);
    } else {
      method = "PATCH";
      payloadToSend = selectedPayload;
      validateEffectiveUpdate(selectedPayload, lastLoadedFlight);
    }

    setMsg(`Saving (${method})...`);

    await fetchJson(`/api/flights/${id}`, {
      method,
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payloadToSend),
    });

    await loadFlightIntoForm(id);
    await loadFlightList(id);

    setMsg(`Flight #${id} updated successfully (${method}).`);
  } catch (e) {
    setMsg(e.message, true);
  }
});

async function boot() {
  await Promise.all([loadPlanes(), loadAirports(), loadAirlines()]);
  await loadFlightList();
  applyCheckboxState();

  const first = flightSelect.value;
  if (first) await loadFlightIntoForm(first);
}

boot().catch((e) => setMsg(e.message, true));