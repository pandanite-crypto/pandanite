FROM ubuntu:22.04

RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get install -y --no-install-recommends \
  ca-certificates \
  curl nano wget htop iputils-ping dnsutils \
  build-essential \
  make cmake automake libtool libleveldb-dev \
  && apt-get clean

RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get install -y --no-install-recommends \
  python3 python3-pip \
  && apt-get clean

RUN update-alternatives --install /usr/bin/python python /usr/bin/python3.10 1
RUN pip3 install conan

WORKDIR /bamboo

COPY src src
COPY blacklist.txt CMakeLists.txt conanfile.txt genesis.json .

WORKDIR /bamboo/build
RUN conan install .. --build=missing

WORKDIR /bamboo
RUN cmake .

RUN make miner
RUN make keygen
RUN make server

RUN cp /bamboo/bin/* /usr/local/bin
