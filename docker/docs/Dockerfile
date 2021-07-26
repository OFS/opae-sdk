FROM ubuntu:latest
RUN apt-get update -y
RUN apt-get install -y apt-utils
RUN apt-get install -y tzdata && dpkg-reconfigure --frontend noninteractive tzdata
RUN apt-get install -y build-essential cmake git libjson-c-dev uuid-dev git \
	               python3-dev python3-wheel python3-pip doxygen libedit-dev \
		       libcap-dev libudev-dev
WORKDIR /root
ENTRYPOINT [ "/bin/bash", "-c" ]


