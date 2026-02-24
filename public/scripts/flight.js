let filteredFlights = [];

const searchInput = document.getElementById("flightSearch");
const dateInput = document.getElementById("dateSearch");
const sortSelect = document.getElementById("sortSelect");
const clearBtn = document.getElementById("clearBtn");
const dropdown = document.getElementById("searchDropdown");

// Pagination elements
const prevBtn = document.getElementById("prevBtn");
const nextBtn = document.getElementById("nextBtn");
const pageInfo = document.getElementById("pageInfo");

let currentPage = 1;
let totalPages = 1;
const pageSize = 100;

// Fetch flights from server (filtered & sorted)
function fetchFlights(renderPage = true) {
  const search = encodeURIComponent(searchInput.value.trim());
  const date = encodeURIComponent(dateInput.value);
  const sort = encodeURIComponent(sortSelect.value);

  let url = `/api/flights?search=${search}&sort=${sort}&date=${date}&page=${currentPage}`;

  fetch(url)
    .then(res => res.json())
    .then(data => {
      filteredFlights = data.flights;

      //read pagination metadata from backend
      currentPage = data.page ?? currentPage;
      totalPages = data.totalPages ?? totalPages;

      if (renderPage) renderFlights(filteredFlights);

      // update pagination UI
      renderPagination();

      if (searchInput.value.trim()) {
        renderDropdown(filteredFlights);
      } else {
        dropdown.style.display = "none";
      }
    });
}

function renderPagination() {
  const pageNumbers = document.getElementById("pageNumbers");

  prevBtn.disabled = currentPage <= 1;
  nextBtn.disabled = currentPage >= totalPages;

  pageInfo.textContent = ` ${currentPage} / ${totalPages}`;

  pageNumbers.innerHTML = "";

  // how many numbers to show around current page
  const windowSize = 2;

  const pages = [];

  const addPage = (p) => pages.push(p);
  const addDots = () => pages.push("...");

  addPage(1);

  let start = Math.max(2, currentPage - windowSize);
  let end = Math.min(totalPages - 1, currentPage + windowSize);

  if (start > 2) addDots();

  for (let p = start; p <= end; p++) addPage(p);

  if (end < totalPages - 1) addDots();

  if (totalPages > 1) addPage(totalPages);

  pages.forEach(p => {
    if (p === "...") {
      const span = document.createElement("span");
      span.className = "dots";
      span.textContent = "...";
      pageNumbers.appendChild(span);
      return;
    }

    const btn = document.createElement("button");
    btn.textContent = p;
    btn.className = "page-btn";
    if (p === currentPage) btn.classList.add("active");

    btn.onclick = () => {
      currentPage = p;
      fetchFlights(true);
      window.scrollTo({ top: 0, behavior: "smooth" });
    };

    pageNumbers.appendChild(btn);
  });
}

document.getElementById("prevBtn").addEventListener("click", () => {
  if (currentPage > 1) {
    currentPage--;
    fetchFlights(true);
    window.scrollTo({ top: 0, behavior: "smooth" });
  }
});

document.getElementById("nextBtn").addEventListener("click", () => {
  if (currentPage < totalPages) {
    currentPage++;
    fetchFlights(true);
    window.scrollTo({ top: 0, behavior: "smooth" });
  }
});

document.getElementById("pageJumpBtn").addEventListener("click", () => {
  const input = document.getElementById("pageJumpInput");
  let p = parseInt(input.value, 10);
  if (Number.isNaN(p)) return;
  p = Math.max(1, Math.min(totalPages, p));
  currentPage = p;
  fetchFlights(true);
  window.scrollTo({ top: 0, behavior: "smooth" });
});

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
    item.textContent = `${f.airline?.name ?? "Airline"} — ${f.origin.code} → ${f.destination.code}`;

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

function formatDateTime(iso) {
  if (!iso) return "—";
  const d = new Date(iso);
  if (isNaN(d.getTime())) return "—";
  return d.toLocaleString(undefined, {
    year: "numeric",
    month: "short",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit"
  });
}

// Render flight cards
function renderFlights(flights) {
  const container = document.getElementById("flightList");
  container.innerHTML = "";

  if (!flights.length) {
    container.innerHTML = `<div class="empty">No flights found for this page.</div>`;
    return;
  }

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
              style="left:${(f.progress ?? 0) * 100}%">
            <div class="plane-name">${f.plane}</div>
          </div>

          <div class="city">
            <strong>${f.destination.city} (${f.destination.code})</strong>
            <div class="weather" id="w-dest-${f.flightID}">Loading weather…</div>
          </div>
        </div>

        <div class="meta">
          Take off: ${formatDateTime(f.departureTime)} |
          Gate: ${f.gate} |
          Passengers: ${f.passengers} |
          Flight time: ${f.durationText ?? ""} |
          Arriving at: ${formatDateTime(f.arrivalTime)}
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

// Load weather
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
searchInput.addEventListener("input", () => {
  currentPage = 1;
  fetchFlights(false);
});

dateInput.addEventListener("change", () => {
  currentPage = 1;
  fetchFlights(true);
});

sortSelect.addEventListener("change", () => {
  currentPage = 1;
  fetchFlights(true);
});


clearBtn.addEventListener("click", () => {
  searchInput.value = "";
  dateInput.value = "";
  sortSelect.value = "status";
  filteredFlights = [];
  dropdown.style.display = "none";
  currentPage = 1;
  fetchFlights(true);
});

document.addEventListener("click", e => {
  if (!e.target.closest(".search-box")) dropdown.style.display = "none";
});

// Initial load
fetchFlights(true);