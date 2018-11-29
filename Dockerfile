FROM ubuntu:18.04

WORKDIR /unit-dmkit

COPY . /unit-dmkit

RUN apt-get update && apt-get install -y --no-install-recommends sudo cmake wget vim curl ca-certificates

RUN update-ca-certificates

RUN sh deps.sh ubuntu

RUN rm -rf _build && mkdir _build && cd _build && cmake .. && make -j8

EXPOSE 8010

