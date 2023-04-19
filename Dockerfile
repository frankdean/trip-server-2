# -*- mode: dockerfile; -*- vim: set ft=dockerfile:
FROM  debian:bullseye-slim AS build
RUN groupadd --gid 1000 trip \
    && useradd --uid 1000 --gid trip --shell /bin/bash --create-home trip
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    procps locales coreutils tar xz-utils ca-certificates tzdata \
    make g++ gawk \
    libboost-locale-dev libpqxx-dev libpugixml-dev libyaml-cpp-dev \
    nlohmann-json3-dev uuid-dev \
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
ARG TRIP_SERVER_VERSION=2.0.0-alpha.38
ARG TRIP_SERVER_SHA256=b148fdc951b1a60e5595c3808415cfe464b7580a501217698992e6259d11b0ed
ARG TRIP_SERVER_FILENAME=trip-${TRIP_SERVER_VERSION}.tar.xz
RUN chgrp trip /usr/local/src && \
    chmod g+w /usr/local/src

USER trip
WORKDIR /usr/local/src
ADD --chown=trip:trip https://www.fdsd.co.uk/trip-server-2/download/${TRIP_SERVER_FILENAME} .
RUN echo "$TRIP_SERVER_SHA256 *${TRIP_SERVER_FILENAME}" | sha256sum -c -
RUN tar -xf $TRIP_SERVER_FILENAME

WORKDIR "/usr/local/src/trip-${TRIP_SERVER_VERSION}"
RUN ./configure --disable-gdal && make check

USER root
RUN make install
# RUN rm -rf "/usr/local/src/trip-${TRIP_SERVER_VERSION}"

WORKDIR /usr/local/etc

FROM  debian:bullseye-slim

RUN groupadd --gid 1000 trip \
    && useradd --uid 1000 --gid trip --shell /bin/bash --create-home trip

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    locales tzdata \
    libboost-locale1.74.0 libpugixml1v5 libyaml-cpp0.6 \
    uuid libpqxx-6.4 \
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

# COPY --from=build /usr/local/bin/trip-server /usr/local/bin/
# COPY --from=build /usr/local/share/trip/ /usr/local/share/trip/

COPY --from=build /usr/local/bin/ /usr/local/bin/
COPY --from=build /usr/local/etc/ /usr/local/etc/
COPY --from=build /usr/local/share/ /usr/local/share/
# COPY --from=build /usr/local/share/locale/es/LC_MESSAGES/trip.mo /usr/local/share/locale/es/LC_MESSAGES/
# COPY --from=build /usr/local/share/locale/en_GB/LC_MESSAGES/trip.mo /usr/local/share/locale/en_GB/LC_MESSAGES/
# COPY --from=build /usr/local/share/locale/fr/LC_MESSAGES/trip.mo /usr/local/share/locale/fr/LC_MESSAGES/
# COPY --from=build /usr/local/share/doc/trip/ /usr/local/share/doc/trip/
# COPY --from=build /usr/local/share/info/trip-*.info /usr/local/share/info/

COPY docker-entrypoint.sh /usr/local/bin/
WORKDIR /usr/local/etc
RUN rm -f trip-server.yaml && touch trip-server.yaml && \
    chown trip:trip trip-server.yaml && chmod 0640 trip-server.yaml

USER trip

ENTRYPOINT ["docker-entrypoint.sh"]

CMD ["trip-server"]
