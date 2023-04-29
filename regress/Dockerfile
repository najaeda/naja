from ubuntu:latest as naja_builder
RUN apt-get update && apt-get -y install \
    git-all \
    cmake make  \
    g++ \
    libboost-dev \
    libfl-dev capnproto libcapnp-dev
COPY / /
RUN cmake . && make -j$(nproc)

from gcr.io/hdl-containers/yosys:latest
COPY --from=naja_builder /src/snl/snippets/app/src/snl_vrl_snippet ./
COPY regress/ regress/
WORKDIR regress
CMD ["make"]
