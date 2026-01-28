fetch("/api/flights")
  .then(res => res.json())
  .then(data => renderFlights(data.flights));

function getStatus(departureTime) {
  const dep = new Date(departureTime);
  const now = new Date();

  if (dep < now) return { text: "DEPARTED", cls: "departed" };

  const diff = (dep - now) / (1000 * 60);
  if (diff < 30) return { text: "BOARDING", cls: "boarding" };

  return { text: "ON TIME", cls: "ontime" };
}

function renderFlights(flights) {
  const container = document.getElementById("flightList");
  container.innerHTML = "";

  flights.forEach(f => {
    const status = getStatus(f.departureTime);

    const card = document.createElement("div");
    card.className = "flight-card";

    card.innerHTML = `
      <div class="flight-header">
        <strong>${f.airline}</strong>
        <span class="status ${status.cls}">${status.text}</span>
      </div>
      <div class="route">
        ${f.originCity} (${f.originCode})
        → ${f.destCity} (${f.destCode})
      </div>
      <div class="meta">
        ${new Date(f.departureTime).toLocaleString()} <br/>
        Gate ${f.gate} | Plane ${f.plane} <br/>
        ${f.passengers} passengers
      </div>
    `;

    container.appendChild(card);
  });
}
