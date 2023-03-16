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
RUN pip3 install conan==1.59

WORKDIR /pandanite

COPY src src
COPY CMakeLists.txt conanfile.txt ./

WORKDIR /pandanite/build
RUN conan install .. --build=missing

WORKDIR /pandanite

RUN cmake .

RUN MAKEFLAGS=-j$(nproc); export MAKEFLAGS; make miner
RUN MAKEFLAGS=-j$(nproc); export MAKEFLAGS; make keygen
RUN MAKEFLAGS=-j$(nproc); export MAKEFLAGS; make server

RUN cp /pandanite/bin/* /usr/local/bin

COPY genesis.json blacklist.txt ./
