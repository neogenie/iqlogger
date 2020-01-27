ARG TAG=ubuntu18

FROM neogenie/iqlogger_builder:$TAG

ARG TAG

ADD . /iqlogger
WORKDIR iqlogger

RUN .build/build-${TAG}.sh

RUN mkdir -p build
RUN cp iqlogger build/

WORKDIR build

#ENTRYPOINT /bin/bash
