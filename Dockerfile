FROM ubuntu:20.04
ARG APPDIR=/usr/local/calc
WORKDIR ${APPDIR}

# copy my single server program file
COPY build/bin/svr ./
COPY build/cli     ./cli

# install openssl into the image
RUN apt-get update && apt-get install -y libssl1.1 && apt clean && rm -rf /var/lib/apt/lists/*

# run the server program
EXPOSE 1080/tcp
EXPOSE 1443/tcp
ENV CALC_SITE_HOME=${APPDIR}/cli
CMD ./svr
