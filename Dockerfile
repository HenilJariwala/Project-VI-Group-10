FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    g++ make \
    libasio-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN make

EXPOSE 18080
CMD ["./server"]
