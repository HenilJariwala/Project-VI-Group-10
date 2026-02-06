let flightsData = [];
let filteredFlights = [];

const statusOrder = {
  boarding: 0,
  ontime: 1,
  departed: 2
};

const searchInput = document.getElementById("flightSearch");
const dateInput = document.getElementById("dateSearch");
const sortSelect = document.getElementById("sortSelect");
const clearBtn = document.getElementById("clearBtn");
const dropdown = document.getElementById("searchDropdown");

function getStatus(departureTime) {
  const dep = new Date(departureTime);
  const now = new Date();

  if (dep < now) return { text: "DEPARTED", cls: "departed" };

  const diff = (dep - now) / (1000 * 60);
  if (diff < 30) return { text: "BOARDING", cls: "boarding" };

  return { text: "ON TIME", cls: "ontime" };
}

function sortFlights(flights) {
  if (sortSelect.value === "departure") {
    return [...flights].sort(
      (a, b) => new Date(a.departureTime) - new Date(b.departureTime)
    );
  }

  if (sortSelect.value === "gate") {
    return [...flights].sort((a, b) => a.gate.localeCompare(b.gate));
  }

  return [...flights].sort((a, b) => {
    const sA = getStatus(a.departureTime).cls;
    const sB = getStatus(b.departureTime).cls;
    return statusOrder[sA] - statusOrder[sB];
  });
}

function applyDateAndSort(base) {
  let result = [...base];

  if (dateInput.value) {
    const selectedTime = new Date(dateInput.value).getTime();
    result = result.filter(f => {
      const flightTime = new Date(f.departureTime).getTime();
      return Math.abs(flightTime - selectedTime) <= 24 * 60 * 60 * 1000;
    });
  }

  return sortFlights(result);
}

fetch("/api/flights")
  .then(res => res.json())
  .then(data => {
    flightsData = data.flights;
    filteredFlights = [...flightsData];
    renderFlights(sortFlights(filteredFlights));
  });

searchInput.addEventListener("input", () => {
  const q = searchInput.value.toLowerCase().trim();

  if (!q) {
    dropdown.style.display = "none";
    return;
  }

  const matches = flightsData.filter(f =>
    f.airline.toLowerCase().includes(q) ||
    f.origin.city.toLowerCase().includes(q) ||
    f.origin.code.toLowerCase().includes(q) ||
    f.destination.city.toLowerCase().includes(q) ||
    f.destination.code.toLowerCase().includes(q) ||
    f.gate.toLowerCase().includes(q) ||
    f.plane.toLowerCase().includes(q)
  );

  renderDropdown(matches);
});

dateInput.addEventListener("change", () => {
  renderFlights(applyDateAndSort(filteredFlights.length ? filteredFlights : flightsData));
});

sortSelect.addEventListener("change", () => {
  renderFlights(sortFlights(filteredFlights.length ? filteredFlights : flightsData));
});

clearBtn.addEventListener("click", () => {
  searchInput.value = "";
  dateInput.value = "";
  sortSelect.value = "status";
  dropdown.style.display = "none";
  filteredFlights = [...flightsData];
  renderFlights(sortFlights(filteredFlights));
});

function renderDropdown(flights) {
  dropdown.innerHTML = "";

  if (!searchInput.value.trim()) {
    dropdown.style.display = "none";
    return;
  }

  flights.slice(0, 6).forEach(f => {
    const item = document.createElement("div");
    item.className = "search-item";
    item.textContent = `${f.airline} — ${f.origin.code} → ${f.destination.code}`;

    item.onclick = () => {
      searchInput.value = `${f.origin.code} → ${f.destination.code}`;
      dropdown.style.display = "none";
      filteredFlights = [f];
      renderFlights(applyDateAndSort(filteredFlights));
    };

    dropdown.appendChild(item);
  });

  dropdown.style.display = flights.length ? "block" : "none";
}

document.addEventListener("click", e => {
  if (!e.target.closest(".search-box")) {
    dropdown.style.display = "none";
  }
});

function renderFlights(flights) {
  const container = document.getElementById("flightList");
  container.innerHTML = "";

  flights.forEach(f => {
    const status = getStatus(f.departureTime);
    const dep = new Date(f.departureTime);
    const now = new Date();
    const progress = Math.min(
      Math.max((now - (dep - 30 * 60 * 1000)) / (30 * 60 * 1000), 0),
      1
    );

    const card = document.createElement("div");
    card.innerHTML = `
      <div class="flight-card">
        <div class="airline-title">
          <span>${f.airline}</span>
        </div>

        <div class="route-row">
          <div class="city">
            <strong>${f.origin.city} (${f.origin.code})</strong>
            <div class="weather" id="w-origin-${f.flightID}">Loading weather…</div>
          </div>

          <div class="route-line">
            <div class="line"></div>
            <img src="/assets/plane.gif" 
              class="plane-gif" 
              data-flight-id="${f.flightID}" 
              style="left:${progress * 100}%">
            <div class="plane-name">${f.plane}</div>
          </div>

          <div class="city">
            <strong>${f.destination.city} (${f.destination.code})</strong>
            <div class="weather" id="w-dest-${f.flightID}">Loading weather…</div>
          </div>
        </div>

        <div class="meta">
          ${new Date(f.departureTime).toLocaleString()} |
          Gate ${f.gate} |
          ${f.passengers} passengers
        </div>

        <div class="status ${status.cls}" data-flight-id="${f.flightID}">
          ${status.text}
        </div>

      </div>
    `;

    container.appendChild(card);

    loadWeather(f.origin.latitude, f.origin.longitude, `w-origin-${f.flightID}`);
    loadWeather(f.destination.latitude, f.destination.longitude, `w-dest-${f.flightID}`);
  });
}

function updateFlightProgressAndStatus() {
  const now = new Date();

  document.querySelectorAll(".plane-gif").forEach(plane => {
    const flightID = plane.dataset.flightId;
    const flight = filteredFlights.find(f => f.flightID == flightID);
    if (!flight) return;

    const dep = new Date(flight.departureTime);

    // --- Update plane progress ---
    let progress = Math.min(Math.max((now - (dep - 30 * 60 * 1000)) / (30 * 60 * 1000), 0), 1);
    plane.style.left = `${progress * 100}%`;

    // --- Update status ---
    const statusEl = document.querySelector(`.status[data-flight-id="${flightID}"]`);
    if (!statusEl) return;

    const diff = (dep - now) / (1000 * 60); // difference in minutes
    let statusCls, statusText;

    if (diff < 0) {
      statusCls = "departed";
      statusText = "DEPARTED";
    } else if (diff < 30) {
      statusCls = "boarding";
      statusText = "BOARDING";
    } else {
      statusCls = "ontime";
      statusText = "ON TIME";
    }

    // Only update if changed
    if (statusEl.className !== `status ${statusCls}`) {
      statusEl.className = `status ${statusCls}`;
      statusEl.textContent = statusText;
    }
  });
}

// Update every 5 seconds
setInterval(updateFlightProgressAndStatus, 5000);


function loadWeather(lat, lon, elementId) {
  fetch(`https://api.open-meteo.com/v1/forecast?latitude=${lat}&longitude=${lon}&current_weather=true`)
    .then(res => res.json())
    .then(data => {
      const el = document.getElementById(elementId);
      if (!el || !data.current_weather) return;
      const w = data.current_weather;
      el.innerText = `${w.temperature}°C, ${weatherCodeToText(w.weathercode)}`;
    })
    .catch(() => {
      const el = document.getElementById(elementId);
      if (el) el.innerText = "Weather unavailable";
    });
}

function weatherCodeToText(code) {
  if (code === 0) return "Clear ☀️";
  if (code <= 3) return "Partly Cloudy 🌤️";
  if (code <= 48) return "Fog 🌫️";
  if (code <= 67) return "Rain 🌧️";
  if (code <= 77) return "Snow ❄️";
  return "Storm ⛈️";
}
