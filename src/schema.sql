PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS Cities (
  cityID INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL,
  latitude REAL NOT NULL,
  longitude REAL NOT NULL
);

CREATE TABLE IF NOT EXISTS Airport (
  airportID INTEGER PRIMARY KEY AUTOINCREMENT,
  cityID INTEGER NOT NULL,
  code TEXT NOT NULL,
  FOREIGN KEY (cityID) REFERENCES Cities(cityID)
);

CREATE TABLE IF NOT EXISTS Plane (
  planeID INTEGER PRIMARY KEY AUTOINCREMENT,
  model TEXT NOT NULL,
  speed INTEGER NOT NULL,
  maxSeats INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS Airline (
  airlineID INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL UNIQUE,
  logoPath TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS Flight (
  flightID INTEGER PRIMARY KEY AUTOINCREMENT,
  planeID INTEGER NOT NULL,
  originAirportID INTEGER NOT NULL,
  destinationAirportID INTEGER NOT NULL,
  airlineID INTEGER NOT NULL,
  gate TEXT NOT NULL,
  passengerCount INTEGER NOT NULL,
  departureTime TEXT NOT NULL,
  FOREIGN KEY (planeID) REFERENCES Plane(planeID),
  FOREIGN KEY (originAirportID) REFERENCES Airport(airportID),
  FOREIGN KEY (destinationAirportID) REFERENCES Airport(airportID),
  FOREIGN KEY (airlineID) REFERENCES Airline(airlineID)
);



CREATE INDEX IF NOT EXISTS idx_flight_departureTime ON Flight(departureTime);
CREATE INDEX IF NOT EXISTS idx_flight_gate ON Flight(gate);
CREATE INDEX IF NOT EXISTS idx_flight_airlineID ON Flight(airlineID);
CREATE INDEX IF NOT EXISTS idx_flight_originAirportID ON Flight(originAirportID);
CREATE INDEX IF NOT EXISTS idx_flight_destinationAirportID ON Flight(destinationAirportID);