import os
import time
import requests
import pytest


def _base_url() -> str:
    return os.getenv("BASE_URL", "http://localhost:18080").rstrip("/")


@pytest.fixture(scope="session")
def base_url() -> str:
    return _base_url()


@pytest.fixture(scope="session")
def http() -> requests.Session:
    s = requests.Session()
    yield s
    s.close()


def _pick_first_id(obj_list, possible_keys):
    """
    Tries common ID keys; returns the first integer ID found.
    """
    if not isinstance(obj_list, list) or not obj_list:
        raise RuntimeError("Expected a non-empty list from API.")
    first = obj_list[0]
    if not isinstance(first, dict):
        raise RuntimeError("Expected list elements to be objects/dicts.")
    for k in possible_keys:
        if k in first and isinstance(first[k], int):
            return first[k]
    raise RuntimeError(f"Could not find an int ID in object. Keys: {list(first.keys())}")


@pytest.fixture(scope="session")
def reference_ids(base_url, http):
    """
    Fetches valid planeID, airlineID, originAirportID, destinationAirportID for use in tests.
    This makes the suite work even if seed IDs change.
    """
    planes = http.get(f"{base_url}/api/planes", timeout=10)
    planes.raise_for_status()
    planes_json = planes.json()
    # expecting { "planes": [...] }
    plane_id = _pick_first_id(planes_json.get("planes", []), ["planeID", "id", "planeId"])

    airlines = http.get(f"{base_url}/api/airlines", timeout=10)
    airlines.raise_for_status()
    airlines_json = airlines.json()
    airline_id = _pick_first_id(airlines_json.get("airlines", []), ["airlineID", "id", "airlineId"])

    airports = http.get(f"{base_url}/api/airports", timeout=10)
    airports.raise_for_status()
    airports_json = airports.json()
    airport_list = airports_json.get("airports", [])
    if len(airport_list) < 2:
        raise RuntimeError("Need at least 2 airports in DB for origin/destination.")
    origin_id = _pick_first_id(airport_list, ["airportID", "id", "airportId"])
    dest_id = _pick_first_id(airport_list[1:], ["airportID", "id", "airportId"])

    if origin_id == dest_id:
        raise RuntimeError("Origin and destination airport IDs must be different for API validation.")

    return {
        "planeID": plane_id,
        "airlineID": airline_id,
        "originAirportID": origin_id,
        "destinationAirportID": dest_id,
    }


@pytest.fixture
def new_flight_payload(reference_ids):
    """
    Valid POST/PUT payload based on your API contract.
    departureTime format used across your code: YYYY-MM-DDTHH:MM:SS (no Z).
    """
    # use a time-based gate to reduce collisions in shared DBs
    gate = f"PY{int(time.time()) % 100000}"
    return {
        "planeID": reference_ids["planeID"],
        "airlineID": reference_ids["airlineID"],
        "originAirportID": reference_ids["originAirportID"],
        "destinationAirportID": reference_ids["destinationAirportID"],
        "gate": gate,
        "passengerCount": 123,
        "departureTime": "2026-02-20T12:00:00",
    }


@pytest.fixture
def created_flight_id(base_url, http, new_flight_payload):
    """
    Creates a flight via POST and yields its flightID.
    Always cleans up by deleting the created flight afterward.
    """
    r = http.post(f"{base_url}/api/flights", json=new_flight_payload, timeout=10)
    assert r.status_code == 201, f"Expected 201, got {r.status_code}: {r.text}"
    data = r.json()
    assert "flightID" in data and isinstance(data["flightID"], int), f"Bad response: {data}"
    flight_id = data["flightID"]

    yield flight_id

    # cleanup
    http.delete(f"{base_url}/api/flights/{flight_id}", timeout=10)