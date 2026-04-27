FROM ubuntu:jammy

# Install dependencies
RUN apt-get update \
    && apt-get install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib build-essential \
    && apt-get install -y git python3 python3-venv rsync \
    && rm -rf /var/lib/apt/lists/* && apt-get autoremove && apt-get autoclean

# "Install" the Pico SDK
RUN mkdir /pico && cd /pico \
    && git clone https://github.com/raspberrypi/pico-sdk.git --branch master \
    && cd pico-sdk \
    && git submodule update --init

# Set Pico SDK path & compiler
ENV PICO_SDK_PATH=/pico/pico-sdk
ENV PICO_COMPILER=pico_arm_gcc

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
