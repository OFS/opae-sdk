FROM ubuntu:latest
RUN apt-get update
RUN apt-get install apt-utils
RUN apt-get install -y tzdata && dpkg-reconfigure --frontend noninteractive tzdata
RUN apt-get install -y build-essential cmake git libjson-c-dev uuid-dev git python3-dev python3-wheel python3-pip doxygen
WORKDIR /root
ENTRYPOINT [ "/bin/bash", "-c" ]


