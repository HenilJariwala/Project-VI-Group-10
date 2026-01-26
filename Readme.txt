##Flightlogger for proj 6

##built

navigate to project directory and open powershell
do:
docker build -t flight-logger .
docker run --rm -p 18080:18080 flight-logger
then go to: http://localhost:18080/
