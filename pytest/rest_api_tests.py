import requests


# ---------------------------
# Functional Tests (FUNC-API-xx)
# ---------------------------

def test_FUNC_API_01_get_flights_list(base_url, http):
    r = http.get(f"{base_url}/api/flights", timeout=10)
    assert r.status_code == 200
    data = r.json()
    assert isinstance(data, dict)
    assert "flights" in data
    assert isinstance(data["flights"], list)


def test_FUNC_API_02_get_flight_by_id_returns_correct_data(base_url, http, created_flight_id, new_flight_payload):
    r = http.get(f"{base_url}/api/flights/{created_flight_id}", timeout=10)
    assert r.status_code == 200
    flight = r.json()
    assert isinstance(flight, dict)
    # getFlightById returns raw fields used by PUT/PATCH logic
    assert flight.get("flightID") == created_flight_id
    assert flight.get("planeID") == new_flight_payload["planeID"]
    assert flight.get("airlineID") == new_flight_payload["airlineID"]
    assert flight.get("originAirportID") == new_flight_payload["originAirportID"]
    assert flight.get("destinationAirportID") == new_flight_payload["destinationAirportID"]
    assert flight.get("gate") == new_flight_payload["gate"]
    assert flight.get("passengerCount") == new_flight_payload["passengerCount"]
    assert flight.get("departureTime") == new_flight_payload["departureTime"]


def test_FUNC_API_03_post_create_flight_succeeds(base_url, http, new_flight_payload):
    r = http.post(f"{base_url}/api/flights", json=new_flight_payload, timeout=10)
    assert r.status_code == 201, r.text
    data = r.json()
    assert data.get("message") == "Flight created"
    assert isinstance(data.get("flightID"), int)

    # cleanup
    http.delete(f"{base_url}/api/flights/{data['flightID']}", timeout=10)


def test_FUNC_API_04_put_update_flight_succeeds(base_url, http, created_flight_id, new_flight_payload):
    updated = dict(new_flight_payload)
    updated["gate"] = new_flight_payload["gate"] + "-UPD"
    updated["passengerCount"] = new_flight_payload["passengerCount"] + 1

    r = http.put(f"{base_url}/api/flights/{created_flight_id}", json=updated, timeout=10)
    assert r.status_code == 200, r.text
    data = r.json()
    assert data.get("message") == "Flight updated"
    assert data.get("flightID") == created_flight_id

    # verify persisted
    g = http.get(f"{base_url}/api/flights/{created_flight_id}", timeout=10)
    assert g.status_code == 200
    flight = g.json()
    assert flight.get("gate") == updated["gate"]
    assert flight.get("passengerCount") == updated["passengerCount"]


def test_FUNC_API_05_delete_flight_removes_existing(base_url, http, new_flight_payload):
    # create a flight first
    r = http.post(f"{base_url}/api/flights", json=new_flight_payload, timeout=10)
    assert r.status_code == 201
    flight_id = r.json()["flightID"]

    # delete the flight
    d = http.delete(f"{base_url}/api/flights/{flight_id}", timeout=10)
    assert d.status_code == 200, d.text
    data = d.json()
    assert data.get("message") == "Flight deleted"
    assert data.get("flightID") == flight_id

    # confirm it's gone
    g = http.get(f"{base_url}/api/flights/{flight_id}", timeout=10)
    assert g.status_code == 404


# ---------------------------
# Integration Tests (INT-API-xx)
# ---------------------------

def test_INT_API_01_list_contains_retrievable_flights(base_url, http):
    """
    Integration check: list endpoint returns flight IDs that can be fetched individually.
    We only sample up to 3 to keep the test fast.
    """
    r = http.get(f"{base_url}/api/flights", timeout=10)
    assert r.status_code == 200
    flights = r.json().get("flights", [])
    assert isinstance(flights, list)

    sample = flights[:3]
    for f in sample:
        assert "flightID" in f and isinstance(f["flightID"], int)
        g = http.get(f"{base_url}/api/flights/{f['flightID']}", timeout=10)
        assert g.status_code == 200


def test_INT_API_02_post_then_get_round_trip(base_url, http, new_flight_payload):
    """
    Integration: POST persists -> GET returns same payload fields.
    """
    r = http.post(f"{base_url}/api/flights", json=new_flight_payload, timeout=10)
    assert r.status_code == 201, r.text
    flight_id = r.json()["flightID"]

    g = http.get(f"{base_url}/api/flights/{flight_id}", timeout=10)
    assert g.status_code == 200
    flight = g.json()

    # Compare persisted fields
    assert flight.get("planeID") == new_flight_payload["planeID"]
    assert flight.get("airlineID") == new_flight_payload["airlineID"]
    assert flight.get("originAirportID") == new_flight_payload["originAirportID"]
    assert flight.get("destinationAirportID") == new_flight_payload["destinationAirportID"]
    assert flight.get("gate") == new_flight_payload["gate"]
    assert flight.get("passengerCount") == new_flight_payload["passengerCount"]
    assert flight.get("departureTime") == new_flight_payload["departureTime"]

    # cleanup
    http.delete(f"{base_url}/api/flights/{flight_id}", timeout=10)


def test_INT_API_03_put_then_get_round_trip(base_url, http, created_flight_id, new_flight_payload):
    """
    Integration: PUT persists -> GET reflects updated state.
    """
    updated = dict(new_flight_payload)
    updated["gate"] = new_flight_payload["gate"] + "-IUPD"
    updated["passengerCount"] = 777

    r = http.put(f"{base_url}/api/flights/{created_flight_id}", json=updated, timeout=10)
    assert r.status_code == 200, r.text

    g = http.get(f"{base_url}/api/flights/{created_flight_id}", timeout=10)
    assert g.status_code == 200
    flight = g.json()
    assert flight.get("gate") == updated["gate"]
    assert flight.get("passengerCount") == updated["passengerCount"]


def test_INT_API_04_invalid_input_rejected_no_create(base_url, http, reference_ids, new_flight_payload):
    """
    Integration: invalid input returns 400 and does not create a new record.
    We use the validation rule in your API: originAirportID != destinationAirportID.
    """
    # baseline count (best-effort)
    before = http.get(f"{base_url}/api/flights", timeout=10).json().get("flights", [])
    before_count = len(before) if isinstance(before, list) else None

    bad = dict(new_flight_payload)
    bad["destinationAirportID"] = bad["originAirportID"]  # triggers API validation 400

    r = http.post(f"{base_url}/api/flights", json=bad, timeout=10)
    assert r.status_code == 400
    assert "must be different" in r.text

    after = http.get(f"{base_url}/api/flights", timeout=10).json().get("flights", [])
    if before_count is not None and isinstance(after, list):
        assert len(after) == before_count