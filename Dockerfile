FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    g++ make \
    libasio-dev \
    sqlite3 libsqlite3-dev \
    doxygen graphviz \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN make

EXPOSE 18080
CMD ["bash", "-lc", "mkdir -p /app/doxygen/html && doxygen /app/Doxyfile && ./server"]
