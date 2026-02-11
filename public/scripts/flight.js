let filteredFlights = [];

const searchInput = document.getElementById("flightSearch");
const dateInput = document.getElementById("dateSearch");
const sortSelect = document.getElementById("sortSelect");
const clearBtn = document.getElementById("clearBtn");
const dropdown = document.getElementById("searchDropdown");

// Fetch flights from server (filtered & sorted) 
function fetchFlights(renderPage = true) {
  const search = encodeURIComponent(searchInput.value.trim());
  const date = encodeURIComponent(dateInput.value);
  const sort = encodeURIComponent(sortSelect.value);

  let url = `/api/flights?search=${search}&sort=${sort}&date=${date}`;

  fetch(url)
    .then(res => res.json())
    .then(data => {
      filteredFlights = data.flights;
      
      if (renderPage) {
        renderFlights(filteredFlights);
      }
      
      if (searchInput.value.trim()) {
        renderDropdown(filteredFlights);
      } else {
        dropdown.style.display = "none";
      }
    });
}

// Search dropdown 
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
      renderFlights(filteredFlights);
    };

    dropdown.appendChild(item);
  });

  dropdown.style.display = flights.length ? "block" : "none";
}

// Render flight cards 
function renderFlights(flights) {
  const container = document.getElementById("flightList");
  container.innerHTML = "";

  flights.forEach(f => {
    const card = document.createElement("div");
    card.innerHTML = `
      <div class="flight-card">
        <div class="airline-title">
          <img
            src="${f.airline.logoPath}"
            alt="${f.airline.name}"
            title="${f.airline.name}"
          />
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
              style="left:${f.progress * 100}%">
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

        <div class="status ${f.status.class}" data-flight-id="${f.flightID}">
          ${f.status.text}
        </div>

      </div>
    `;

    container.appendChild(card);

    loadWeather(f.origin.latitude, f.origin.longitude, `w-origin-${f.flightID}`);
    loadWeather(f.destination.latitude, f.destination.longitude, `w-dest-${f.flightID}`);
  });
}

// Updating plane progress & status every 5 seconds
// Now just re-fetches from server instead of recalculating
function updateFlightProgressAndStatus() {
  if (filteredFlights.length > 0) {
    fetchFlights(true);
  }
}

setInterval(updateFlightProgressAndStatus, 5000);

// --- Load weather ---
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

// Event listeners
searchInput.addEventListener("input", () => fetchFlights(false));
dateInput.addEventListener("change", () => fetchFlights(true));
sortSelect.addEventListener("change", () => fetchFlights(true));

clearBtn.addEventListener("click", () => {
  searchInput.value = "";
  dateInput.value = "";
  sortSelect.value = "status";
  filteredFlights = [];
  dropdown.style.display = "none";
  fetchFlights(true);
});

document.addEventListener("click", e => {
  if (!e.target.closest(".search-box")) dropdown.style.display = "none";
});

// Initial load 
fetchFlights(true);