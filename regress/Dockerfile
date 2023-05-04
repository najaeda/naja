FROM gcr.io/hdl-containers/yosys:latest
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
CMD make clean; PYTHONPATH=/install/lib/python LD_LIBRARY_PATH=/install/lib make SNL_X2Y=/src/snl/snippets/cpp/snl_vrl_snippet PRIMITIVES=/primitives/xilinx.py
