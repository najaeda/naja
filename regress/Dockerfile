FROM gcr.io/hdl-containers/verilator:latest AS verilator

FROM gcr.io/hdl-containers/yosys:latest
COPY --from=verilator /usr/local/bin/ /usr/local/bin/
COPY --from=verilator /usr/local/share/verilator/ /usr/local/share/verilator/
RUN apt-get update && apt-get -y install \
    git \
    pkg-config \
    wget make  \
    g++ \
    flex bison \
    libboost-dev \
    python3-dev \
    libfl-dev capnproto libcapnp-dev libtbb-dev
# Put your own Dockfile commands here
RUN rm -rf /var/lib/apt/lists/* \
  && wget https://github.com/Kitware/CMake/releases/download/v3.24.1/cmake-3.24.1-Linux-x86_64.sh \
      -q -O /tmp/cmake-install.sh \
      && chmod u+x /tmp/cmake-install.sh \
      && mkdir /opt/cmake-3.24.1 \
      && /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake-3.24.1 \
      && rm /tmp/cmake-install.sh \
      && ln -s /opt/cmake-3.24.1/bin/* /usr/local/bin
COPY / /naja
RUN rm -rf /naja/build
RUN rm -rf /naja/install
WORKDIR /naja/build
RUN cmake .. -DCMAKE_INSTALL_PREFIX=/naja/install && make -j$(nproc) && make install

WORKDIR /naja/regress
CMD yosys -V; verilator -V; make clean; SET_PYTHONPATH=/naja/install/lib/python LD_LIBRARY_PATH=/naja/install/lib make NAJA_EDIT=/naja/install/bin/naja_edit PRIMITIVES=/naja/primitives/xilinx.py