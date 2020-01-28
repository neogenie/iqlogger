ARG TAG=ubuntu18

FROM neogenie/iqlogger_builder:$TAG

ARG TAG
ARG ARCH
ARG UBUNTU
ARG VERSION
ARG TRAVIS_BUILD_NUMBER
ARG TRAVIS_BRANCH
ARG TRAVIS_COMMIT
ARG TRAVIS_COMMIT_MESSAGE
ARG USER

ADD . /iqlogger
WORKDIR iqlogger

RUN .build/build-${TAG}.sh

RUN cp iqlogger .build_deb/${TAG}/usr/local/bin/
RUN bash .build_deb/create-deb.sh

