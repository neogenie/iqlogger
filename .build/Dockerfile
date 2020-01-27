ARG TAG=ubuntu18

FROM neogenie/iqlogger_builder:$TAG

ARG TAG

ADD . /iqlogger

RUN /iqlogger/.build/build-${TAG}.sh

WORKDIR iqlogger

#ENTRYPOINT /bin/bash
