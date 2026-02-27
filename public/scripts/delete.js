/* Delete Flight page script
 * Uses:
 *  - GET    /api/flights?page=N   (selector list, paginated)
 *  - DELETE /api/flights/{id}     (delete)
 */

const $ = (id) => document.getElementById(id);

const flightSelect = $("flightSelect");
const reloadBtn = $("reloadBtn");
const deleteBtn = $("deleteBtn");
const details = $("details");
const msg = $("msg");

function setMsg(text, isError = false) {
  msg.textContent = text || "";
  msg.classList.toggle("error", !!isError);
}

function setDetails(text) {
  details.textContent = text || "";
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

// Your backend uses page-based pagination with fixed size=100.
// We fetch page=1..totalPages and merge.
async function fetchAllFlightsPaged() {
  const all = [];

  const first = await fetchJson("/api/flights?page=1");
  const firstFlights = first.flights || [];
  all.push(...firstFlights);

  const totalPages = Number(first.totalPages) || 1;

  for (let p = 2; p <= totalPages; p++) {
    const data = await fetchJson(`/api/flights?page=${p}`);
    const flights = data.flights || [];
    all.push(...flights);
  }

  return all;
}

async function loadFlightList(selectFlightId = null) {
  flightSelect.innerHTML = "";
  setDetails("");

  setMsg("Loading flights...");

  const flights = await fetchAllFlightsPaged();

  if (!flights.length) {
    const opt = document.createElement("option");
    opt.value = "";
    opt.textContent = "No flights found";
    flightSelect.appendChild(opt);
    flightSelect.disabled = true;
    deleteBtn.disabled = true;
    setMsg("");
    return;
  }

  // Sort by flightID ascending
  flights.sort((a, b) => Number(a.flightID) - Number(b.flightID));

  flightSelect.disabled = false;
  deleteBtn.disabled = false;

  flights.forEach((f) => {
    const opt = document.createElement("option");
    opt.value = f.flightID;

    const when = (f.departureTime || "").replace("T", " ").slice(0, 16);
    const airlineName = f.airline?.name ?? "Unknown Airline";
    const o = f.origin?.code ?? "???";
    const d = f.destination?.code ?? "???";

    opt.textContent = `#${f.flightID} • ${airlineName} • ${o} → ${d} • ${when}`;

    opt.dataset.airline = airlineName;
    opt.dataset.origin = o;
    opt.dataset.dest = d;
    opt.dataset.when = when;

    flightSelect.appendChild(opt);
  });

  if (selectFlightId) {
    flightSelect.value = String(selectFlightId);
  }

  // If selected ID not present, fall back to first
  if (!flightSelect.value) {
    flightSelect.selectedIndex = 0;
  }

  updateDetails();
  setMsg("");
}

function updateDetails() {
  const opt = flightSelect.selectedOptions?.[0];
  if (!opt || !opt.value) {
    setDetails("");
    return;
  }
  setDetails(
    `Selected: ${opt.dataset.airline} • ${opt.dataset.origin} → ${opt.dataset.dest} • ${opt.dataset.when}`
  );
}

reloadBtn.addEventListener("click", async () => {
  try {
    setMsg("Refreshing...");
    const current = flightSelect.value;
    await loadFlightList(current);
    setMsg("List refreshed.");
  } catch (e) {
    setMsg(e.message, true);
  }
});

flightSelect.addEventListener("change", () => {
  updateDetails();
  setMsg("");
});

deleteBtn.addEventListener("click", async () => {
  try {
    const id = flightSelect.value;
    if (!id) throw new Error("Choose a flight first.");

    const label = flightSelect.selectedOptions?.[0]?.textContent || `Flight #${id}`;
    const ok = window.confirm(`Delete ${label}?\n\nThis cannot be undone.`);
    if (!ok) return;

    setMsg("Deleting...");
    await fetchJson(`/api/flights/${id}`, { method: "DELETE" });

    await loadFlightList(null);
    setMsg(`Flight #${id} deleted.`);
  } catch (e) {
    setMsg(e.message, true);
  }
});

loadFlightList().catch((e) => setMsg(e.message, true));