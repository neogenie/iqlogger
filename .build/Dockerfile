ARG TAG=ubuntu18

FROM neogenie/iqlogger_builder:$TAG

ENV TAG=$TAG

ADD . /iqlogger
WORKDIR iqlogger

RUN .build/build-${TAG}.sh

#ENTRYPOINT /bin/bash
