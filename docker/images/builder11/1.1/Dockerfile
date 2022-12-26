FROM opennetworklinux/builder11:1.0
MAINTAINER Jeffrey Townsend <jeffrey.townsend@bigswitch.com>

RUN apt-get update 
RUN apt --fix-broken install -y
RUN apt-get upgrade -y
#RUN apt-get upgrade -y

COPY multistrap-insecure-fix.patch /tmp
RUN  patch -p1 < /tmp/multistrap-insecure-fix.patch /usr/sbin/multistrap
RUN rm /tmp/multistrap-insecure-fix.patch

#
# Docker shell and other container tools.
#
COPY docker_shell /bin/docker_shell
COPY container-id /bin/container-id
