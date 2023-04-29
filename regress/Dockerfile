from ubuntu:latest as naja_builder
RUN apt-get update && apt-get -y install \
    git \
    pkg-config \
    cmake make  \
    g++ \
    libboost-dev \
    python3-dev \
    libfl-dev capnproto libcapnp-dev
COPY / /
RUN cmake . && make -j$(nproc)

from gcr.io/hdl-containers/yosys:latest
COPY --from=naja_builder /src/snl/snippets/app/src/snl_vrl_snippet ./
COPY regress/ regress/
WORKDIR regress
CMD ["make"]
