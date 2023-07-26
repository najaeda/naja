FROM gcr.io/hdl-containers/verilator:latest AS verilator

FROM gcr.io/hdl-containers/yosys:latest
COPY --from=verilator /usr/local/bin/ /usr/local/bin/
COPY --from=verilator /usr/local/share/verilator/ /usr/local/share/verilator/
RUN apt-get update && apt-get -y install \
    git \
    pkg-config \
    cmake make  \
    g++ \
    flex bison \
    libboost-dev \
    python3-dev \
    libfl-dev capnproto libcapnp-dev
COPY / .
RUN mkdir install
RUN cmake . -DCMAKE_INSTALL_PREFIX=/install && make -j$(nproc) && make install

WORKDIR /regress
CMD make clean; PYTHONPATH=/install/lib/python LD_LIBRARY_PATH=/install/lib make SNL_X2Y=/src/apps/x2y/naja_x2y PRIMITIVES=/primitives/xilinx.py