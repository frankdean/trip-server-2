# -*- mode: dockerfile; -*- vim: set ft=dockerfile:
FROM  debian:bullseye-slim AS build
RUN groupadd --gid 1000 trip \
    && useradd --uid 1000 --gid trip --shell /bin/bash --create-home trip
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    procps locales coreutils tar xz-utils ca-certificates tzdata \
    make g++ gawk \
    libboost-locale-dev libpqxx-dev libpugixml-dev libyaml-cpp-dev \
    nlohmann-json3-dev uuid-dev cairomm-1.0-dev libcmark-dev \
    && rm -rf /var/lib/apt/lists/*

RUN sed -i -e 's/# en_GB.UTF-8 UTF-8/en_GB.UTF-8 UTF-8/' /etc/locale.gen && \
    sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    sed -i -e 's/# es_ES.UTF-8 UTF-8/es_ES.UTF-8 UTF-8/' /etc/locale.gen && \
    sed -i -e 's/# fr_FR.UTF-8 UTF-8/fr_FR.UTF-8 UTF-8/' /etc/locale.gen && \
    locale-gen && \
    export LC_ALL=en_GB.utf8 && \
    localedef -i en_GB -c -f UTF-8 -A /usr/share/locale/locale.alias en_GB.UTF-8 && \
    update-locale LANG LANGUAGE && \
    ln -fs /usr/share/zoneinfo/Europe/London /etc/localtime && \
    dpkg-reconfigure tzdata
ENV LANG en_GB.utf8
ARG TRIP_SERVER_VERSION=2.0.0-alpha.42
ARG TRIP_SERVER_FILENAME=trip-${TRIP_SERVER_VERSION}.tar.xz
RUN chgrp trip /usr/local/src && \
    chmod g+w /usr/local/src

USER trip
WORKDIR /usr/local/src
# ADD --chown=trip:trip https://www.fdsd.co.uk/trip-server-2/download/${TRIP_SERVER_FILENAME} .
# ARG TRIP_SERVER_SHA256=49fec7ba5d0df588594eee4e39c7afeaf5a61b2467f3c8ee62e94541eaddc3ad
# RUN echo "$TRIP_SERVER_SHA256 *${TRIP_SERVER_FILENAME}" | sha256sum -c -

COPY ./${TRIP_SERVER_FILENAME} .
RUN tar -xf $TRIP_SERVER_FILENAME
WORKDIR "/usr/local/src/trip-${TRIP_SERVER_VERSION}"

RUN ./configure --disable-gdal --enable-cairo && make check

USER root
RUN make install

WORKDIR /usr/local/etc

FROM  debian:bullseye-slim

RUN groupadd --gid 1000 trip \
    && useradd --uid 1000 --gid trip --shell /bin/bash --create-home trip

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    locales tzdata \
    libboost-locale1.74.0 libpugixml1v5 libyaml-cpp0.6 \
    uuid libpqxx-6.4 libcairomm-1.0-1v5 libcmark0.29.0 \
    postgresql postgresql-contrib postgis

RUN sed -i -e 's/# en_GB.UTF-8 UTF-8/en_GB.UTF-8 UTF-8/' /etc/locale.gen && \
    sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    sed -i -e 's/# es_ES.UTF-8 UTF-8/es_ES.UTF-8 UTF-8/' /etc/locale.gen && \
    sed -i -e 's/# fr_FR.UTF-8 UTF-8/fr_FR.UTF-8 UTF-8/' /etc/locale.gen && \
    locale-gen && \
    export LC_ALL=en_GB.utf8 && \
    localedef -i en_GB -c -f UTF-8 -A /usr/share/locale/locale.alias en_GB.UTF-8 && \
    update-locale LANG LANGUAGE && \
    ln -fs /usr/share/zoneinfo/Europe/London /etc/localtime && \
    dpkg-reconfigure tzdata
ENV LANG en_GB.utf8

COPY --from=build /usr/local/bin/ /usr/local/bin/
COPY --from=build /usr/local/etc/ /usr/local/etc/
COPY --from=build /usr/local/share/ /usr/local/share/

COPY docker-entrypoint.sh /usr/local/bin/
WORKDIR /usr/local/etc
RUN rm -f trip-server.yaml && touch trip-server.yaml && \
    chown trip:trip trip-server.yaml && chmod 0640 trip-server.yaml

USER trip

ENTRYPOINT ["docker-entrypoint.sh"]

CMD ["trip-server"]
