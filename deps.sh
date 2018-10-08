#!/usr/bin/env bash

cd "$(dirname "$0")"
JOBS=8
OS=$1
case "$OS" in
mac)
    ;;
ubuntu)
    ;;
centos)
    ;;
none)
    ;;
*)
    echo "Usage: $0 [ubuntu|mac|centos|none]"
    exit 1
esac

if [ $OS = mac ]; then
    echo "Installing dependencies for MacOS..."
    brew install openssl git gnu-getopt gflags protobuf leveldb cmake openssl
elif [ $OS = ubuntu ]; then
    echo "Installing dependencies for Ubuntu..."
    sudo apt-get install git \
        g++ \
        make \
        libssl-dev \
        coreutils \
        libgflags-dev \
        libprotobuf-dev \
        libprotoc-dev \
        protobuf-compiler \
        libleveldb-dev \
        libsnappy-dev \
        libcurl4-openssl-dev \
        libgoogle-perftools-dev
elif [ $OS = centos ]; then
    echo "Installing dependencies for CentOS..."
    sudo yum install epel-release
    sudo yum install git gcc-c++ make openssl-devel
    sudo yum install gflags-devel protobuf-devel protobuf-compiler leveldb-devel gperftools-devel
else
    echo "Skipping dependencies installation..."
fi


if [ ! -e brpc ]; then
    echo "Cloning brpc..."
    git clone https://github.com/brpc/brpc.git
fi
cd brpc
git checkout master
git pull
git checkout 2ae7f04ce513c6aee27545df49d5439a98ae3a3f

#build brpc
echo "Building brpc..."
mkdir -p _build
cd _build
cmake ..
make -j$JOBS

cd ../../
