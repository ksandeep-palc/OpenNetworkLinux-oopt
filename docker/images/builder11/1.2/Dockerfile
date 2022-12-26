FROM opennetworklinux/builder11:1.1

MAINTAINER Jeffrey Townsend <jeffrey.townsend@arista.com>

#
# The purpose of this image is to correct the problem in the docker_shell utility described here:
#
# commit d2fbc6ce14f6f1f693d03e2de448620e91842c8c
# Author: Jeffrey Townsend <jeffrey.townsend@bigswitch.com>
# Date:   Mon Mar 23 18:02:46 2020 -0700
#
#    The files in /etc/sudoers.d are ignored if they contain periods. If a invoking username contains a period then the sudo feature is broken and you cannot build.
#

RUN apt-get update 
RUN apt --fix-broken install -y
RUN apt-get upgrade -y

COPY docker_shell /bin/docker_shell
COPY container-id /bin/container-id
RUN apt-get install curl -y
RUN curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py
RUN python2 get-pip.py
RUN pip2 install pyyaml

