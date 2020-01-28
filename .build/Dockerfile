ARG TAG=ubuntu18

FROM neogenie/iqlogger_builder:$TAG

ARG TAG
ARG ARCH
ARG UBUNTU
ARG VERSION

ADD . /iqlogger
WORKDIR iqlogger

RUN .build/build-${TAG}.sh

RUN cp iqlogger .build_deb/${TAG}/usr/local/bin/
RUN bash .build_deb/create-deb.sh

