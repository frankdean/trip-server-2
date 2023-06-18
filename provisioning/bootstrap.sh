#!/usr/bin/env bash

# This file is part of Trip Server 2, a program to support trip recording and
# itinerary planning.
#
# Copyright (C) 2022 Frank Dean <frank.dean@fdsd.co.uk>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Uncomment the following to debug the script
#set -x

LIBPQXX_VERSION=6.4.8
LIBPQXX_SHA256=3f7aba951822e01f1b9f9f353702954773323dd9f9dc376ffb57cb6bbd9a7a2f
NLOHMANN_JSON_VERSION=v3.11.2
NLOHMANN_JSON_SHA256=665fa14b8af3837966949e8eb0052d583e2ac105d3438baba9951785512cf921
NODE_VERSION="v16.20.0"
NODE_SHA256=dff21020b555cc165a1ac36da7d4f6c810b35409c94e00afc51d5d370aae47ae

DOWNLOAD_CACHE_DIR="/vagrant/provisioning/downloads"
LIBPQXX_DOWNLOAD="libpqxx-${LIBPQXX_VERSION}.tar.gz"
LIBPQXX_DOWNLOAD_URL="https://github.com/jtv/libpqxx/archive/refs/tags/${LIBPQXX_VERSION}.tar.gz"
NLOHMANN_JSON_DOWNLOAD_URL="https://github.com/nlohmann/json/releases/download/${NLOHMANN_JSON_VERSION}/json.hpp"

SU_CMD="su vagrant -c"

if [ ! -d "$DOWNLOAD_CACHE_DIR" ]; then
    mkdir -p "$DOWNLOAD_CACHE_DIR"
fi

function install_libpqxx_6
{
    # Download and install libpqxx if it does not exist
    if [ ! -f /usr/local/include/pqxx/pqxx ]; then
	# Does not exist on FreeBSD
	if [ ! -d /usr/local/src ]; then
	    mkdir /usr/local/src
	fi
	if [ ! -d "/usr/local/src/libpqxx-${LIBPQXX_VERSION}" ]; then
	    if [ ! -r "${DOWNLOAD_CACHE_DIR}/LIBPQXX_DOWNLOAD" ]; then
		curl -L --output "${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD}" $LIBPQXX_DOWNLOAD_URL
	    fi
	fi
	if [ -r "${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD}" ]; then
	    echo "$LIBPQXX_SHA256  ${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD}" | shasum -c -
	    if [ $? -ne "0" ]; then
		2>&1 echo "Checksum of ${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD} does not match expected value of ${LIBPQXX_SHA256}"
		exit 1
	    fi
	    cd /usr/local/src
	    mkdir "libpqxx-${LIBPQXX_VERSION}"
	    chown vagrant:vagrant "libpqxx-${LIBPQXX_VERSION}"
	    $SU_CMD "tar -C /usr/local/src -xf ${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD}"
	    cd "libpqxx-${LIBPQXX_VERSION}"
	    pwd
	    if [ -r configure ]; then
		if [ -f /etc/rocky-release ]; then
		    # Weirdly, pkg-config was appending a space to the include
		    # path, presumably due to an incorrect configuration file
		    # which caused 'configure' to fail, apparently unable to
		    # build `libpqxx` by appending a known 'libpq' header
		    # file. Proven by executing the following:
		    # echo "x$(PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/pgsql-13/lib/pkgconfig pkg-config libpq --cflags)x"
		    # So we pedantically specify the lib and include directories
		    PG_LIBDIR=$(/usr/pgsql-13/bin/pg_config --libdir)
		    $SU_CMD "./configure LDFLAGS=-L${PG_LIBDIR} PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:${PG_LIBDIR} --with-postgres-include=$(/usr/pgsql-13/bin/pg_config --includedir) --with-postgres-lib=${PG_LIBDIR} --disable-documentation"
		else
		    $SU_CMD './configure --disable-documentation'
		fi
		$SU_CMD make
		make install
	    else
		echo "configure file is missing"
	    fi
	fi
    fi
}

function install_nlohmann_json
{
    # Download and install nlohmann/json if it does not exist
    if [ ! -r /usr/local/include/nlohmann/json.hpp ]; then
	if [ ! -r "${DOWNLOAD_CACHE_DIR}/nlohmann-json-${NLOHMANN_JSON_VERSION}.hpp" ]; then
	    https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
	    curl -L --output "${DOWNLOAD_CACHE_DIR}/nlohmann-json-${NLOHMANN_JSON_VERSION}.hpp" $NLOHMANN_JSON_DOWNLOAD_URL
	fi
	if [ -r "${DOWNLOAD_CACHE_DIR}/nlohmann-json-${NLOHMANN_JSON_VERSION}.hpp" ]; then
	    echo "$NLOHMANN_JSON_SHA256  ${DOWNLOAD_CACHE_DIR}/nlohmann-json-${NLOHMANN_JSON_VERSION}.hpp" | shasum -c -
	    if [ $? -ne "0" ]; then
		>&2 echo "Checksum of ${DOWNLOAD_CACHE_DIR}/nlohmann-json-${NLOHMANN_JSON_VERSION}.hpp does not match expected value of ${NLOHMANN_JSON_SHA256}"
		exit 1
	    fi
	    mkdir /usr/local/include/nlohmann
	    cp "${DOWNLOAD_CACHE_DIR}/nlohmann-json-${NLOHMANN_JSON_VERSION}.hpp" /usr/local/include/nlohmann/json.hpp
	fi
    fi
}

function install_nodejs
{
    # https://github.com/nodejs/help/wiki/Installation
    NODE_FILENAME="node-${NODE_VERSION}-linux-x64"
    NODE_TAR_FILENAME="${NODE_FILENAME}.tar.xz"
    NODE_EXTRACT_DIR="${NODE_FILENAME}"
    NODE_DOWNLOAD_URL="https://nodejs.org/dist/${NODE_VERSION}/${NODE_TAR_FILENAME}"

    if [ ! -x "/usr/local/lib/nodejs/${NODE_EXTRACT_DIR}/bin/node" ]; then
	# Download the tarball distribution if it does not already exist
	if [ ! -e "${DOWNLOAD_CACHE_DIR}/${NODE_TAR_FILENAME}" ]; then
	    if [ ! -d ${DOWNLOAD_CACHE_DIR} ]; then
		mkdir -p ${DOWNLOAD_CACHE_DIR}
	    fi
	    curl --location --remote-name --output-dir ${DOWNLOAD_CACHE_DIR} $NODE_DOWNLOAD_URL
	fi
	if [ -e "${DOWNLOAD_CACHE_DIR}/$NODE_TAR_FILENAME" ]; then
	    echo "$NODE_SHA256  ${DOWNLOAD_CACHE_DIR}/$NODE_TAR_FILENAME" | shasum -c -
	    if [ $? -ne "0" ]; then
		>&2 echo "Checksum of downloaded file does not match expected value of ${NODE_SHA256}"
		exit 1
	    fi
	    if [ ! -d /usr/local/lib/nodejs ]; then
		mkdir -p /usr/local/lib/nodejs
	    fi
	    tar --no-same-owner --no-same-permissions --xz \
		-xf "${DOWNLOAD_CACHE_DIR}/${NODE_TAR_FILENAME}" \
		-C /usr/local/lib/nodejs
	    if [ -L /usr/local/lib/nodejs/node-current ]; then
		rm -f /usr/local/lib/nodejs/node-current
	    fi
	    ln -sf "/usr/local/lib/nodejs/${NODE_EXTRACT_DIR}" /usr/local/lib/nodejs/node-current
	    if [ -e /vagrant/node_modules ]; then
		cd /vagrant
		rm -rf node_modules
	    fi
	fi
    fi
    grep -E '^export\s+PATH.*nodejs' /home/vagrant/.profile >/dev/null 2>&1
    if [ $? -ne 0 ]; then
	echo "export PATH=/usr/local/lib/nodejs/node-current/bin:$PATH" >>/home/vagrant/.profile
    fi
    if [ -x /usr/local/lib/nodejs/node-current/bin/node ] && \
	   [ ! -x /usr/local/lib/nodejs/node-current/lib/node_modules/yarn/bin/yarn ]; then
	PATH="/usr/local/lib/nodejs/node-current/bin:$PATH" \
	    /usr/local/lib/nodejs/node-current/lib/node_modules/npm/bin/npm-cli.js \
	    install -g yarn
    fi
}

##############################################################################
#
# Fedora & Rocky Linux provisioning
#
##############################################################################

if [ -f /etc/rocky-release ] || [ -f /usr/lib/fedora-release ]; then
    DNF_OPTIONS="--assumeyes"
    dnf $DNF_OPTIONS install gcc gawk boost-devel libpq-devel \
	libpq-devel libuuid-devel \
	curl libX11 libXt libXmu \
	vim autoconf automake info libtool \
	intltool gdb valgrind git \
	nodejs

    if [ -f /etc/rocky-release ]; then
	# Rocky Linux fails to install the VirtualBox Guest Additions kernel with:
	#
	# > Could not find the X.Org or XFree86 Window System, skipping.
	# > modprobe vboxguest failed
	#
	# Installed xorg-x11-server-Xorg manually - do we need to install it?

	# rockylinux: Error: Unable to find a match: postgis autoconf-archive texinfo texinfo-tex yarnpkg

	dnf $DNF_OPTIONS update --refresh
	dnf $DNF_OPTIONS install epel-release
	# https://forums.rockylinux.org/t/how-do-i-install-powertools-on-rocky-linux-9/7427/3
	dnf $DNF_OPTIONS config-manager --set-enabled crb
	dnf $DNF_OPTIONS install gcc gcc-c++ make perl kernel-devel kernel-headers \
	    bzip2 dkms boost-locale autoconf-archive info yarnpkg \
	    texinfo apg \
	    yaml-cpp-devel pugixml-devel screen cmark-devel
	dnf $DNF_OPTIONS update 'kernel-*'

	# postgresql & postgis - https://postgis.net/install/
	dnf $DNF_OPTIONS install https://download.postgresql.org/pub/repos/yum/reporpms/EL-9-x86_64/pgdg-redhat-repo-latest.noarch.rpm
	dnf $DNF_OPTIONS install postgresql13-devel postgis31_13
	# The Rocky Linux box otherwise uninstalls this package with 'dnf autoremove'
	dnf $DNF_OPTIONS mark install NetworkManager-initscripts-updown \
	    grub2-tools-efi python3-configobj rdma-core

    elif [ -f /usr/lib/fedora-release ]; then
	# The Fedora box otherwise uninstalls this package with 'dnf autoremove'
	dnf $DNF_OPTIONS mark install linux-firmware-whence
	dnf $DNF_OPTIONS install postgresql-server postgresql-contrib postgis \
	    autoconf-archive texinfo texinfo-tex \
	    yarnpkg gdal-devel cairomm-devel apg \
	    yaml-cpp-devel pugixml-devel screen cmark-devel
    fi

    install_libpqxx_6
    install_nlohmann_json
    if [ "$VB_GUI" == "y" ]; then
	dnf $DNF_OPTIONS group install lxde-desktop
    fi

    if [ -f /etc/rocky-release ]; then
	if [ ! -r /var/lib/pgsql/13/data/postgresql.conf ]; then
	    /usr/pgsql-13/bin/postgresql-13-setup initdb
	fi
	systemctl is-active postgresql-13 >/dev/null
	if [ $? -ne 0 ]; then
	    systemctl enable postgresql-13
	    systemctl start postgresql-13
	fi
    else
	if [ ! -r /var/lib/pgsql/data/postgresql.conf ]; then
	    postgresql-setup --initdb
	fi
	systemctl is-active postgresql.service >/dev/null
	if [ $? -ne 0 ]; then
	    systemctl enable postgresql.service
	    systemctl start postgresql.service
	fi
    fi

    exit 0;
fi

##############################################################################
#
# FreeBSD provisioning
#
##############################################################################
if [ -x /bin/freebsd-version ]; then
    FREEBSD_PKG_OPTIONS='--yes'
    pkg install $FREEBSD_PKG_OPTIONS pkgconf git boost-all yaml-cpp \
	postgresql13-client postgresql13-contrib postgresql13-server \
	postgis33 \
	python3 pugixml e2fsprogs-libuuid nlohmann-json \
	texinfo vim python3 valgrind apg \
	intltool gdb libtool autoconf-archive gettext automake cairomm \
	cmark \
	node16 yarn-node16
    # Include the textlive-full package to allow building the PDF docs, which
    # needs an additional 11G or more of disk space.
    
    # 8.7G used before adding these (7.8G used after clearing cache).
    #texlive-full

    # Remove the downloaded packages after installation
    #rm -f /var/cache/pkg/*
    
    # Weirdly, on FreeBSD the /vagrant/provisioning folder isn't synced
    DOWNLOAD_CACHE_DIR="/home/vagrant/downloads"
    mkdir -p $DOWNLOAD_CACHE_DIR
    install_libpqxx_6
    if [ "$VB_GUI" == "y" ]; then
	pkg install $FREEBSD_PKG_OPTIONS lxde-meta
    fi
    chsh -s /usr/local/bin/bash vagrant

    if [ ! -d /var/db/postgres/data13/postgresql.conf ]; then
	grep 'postgresql_enable' /etc/rc.conf
	if [ $? -ne 0 ]; then
	    cat >> /etc/rc.conf <<EOF
postgresql_enable="YES"
EOF
	fi
	/usr/local/etc/rc.d/postgresql initdb
	/usr/local/etc/rc.d/postgresql start
	# Note that the socket is created at /tmp/.s.PGSQL.5432
    fi
    exit 0;
fi

##############################################################################
#
# Debian provisioning
#
##############################################################################
if [ -f /etc/debian_version ]; then
    export DEBIAN_FRONTEND=noninteractive
    DEB_OPTIONS="--yes --allow-change-held-packages"
    apt-get update
    apt-get full-upgrade $DEB_OPTIONS

    sed -i -e 's/# en_GB.UTF-8 UTF-8/en_GB.UTF-8 UTF-8/' /etc/locale.gen
    sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen
    sed -i -e 's/# es_ES.UTF-8 UTF-8/es_ES.UTF-8 UTF-8/' /etc/locale.gen
    sed -i -e 's/# fr_FR.UTF-8 UTF-8/fr_FR.UTF-8 UTF-8/' /etc/locale.gen
    locale-gen
    #export LC_ALL=en_GB.utf8
    localedef -i en_GB -c -f UTF-8 -A /usr/share/locale/locale.alias en_GB.UTF-8
    # Set locale to 'none' - locale will default to that of SSH user
    update-locale LANG LANGUAGE

    ln -fs /usr/share/zoneinfo/Europe/London /etc/localtime
    # Other time zones, useful for testing
    #ln -fs /usr/share/zoneinfo/Asia/Kolkata  /etc/localtime # UTC +05:30

    apt-get install -y tzdata
    dpkg-reconfigure tzdata

    apt-get install $DEB_OPTIONS apt-transport-https
    apt-get install $DEB_OPTIONS g++ git postgresql postgresql-contrib postgis \
	    libpqxx-dev screen autoconf autoconf-doc automake autoconf-archive \
	    libtool gettext valgrind uuid-dev uuid-runtime make nginx apg \
	    libboost-locale-dev libpugixml-dev autopoint intltool gdb \
	    libyaml-cpp-dev nlohmann-json3-dev libcmark-dev \
	    docbook2x texlive info texinfo curl libgdal-dev libcairomm-1.0-dev \
	    vim

    if [ ! -d /etc/apt/keyrings ]; then
	install -m 0755 -d /etc/apt/keyrings
    fi
    if [ ! -e /etc/apt/keyrings/docker.gpg ]; then
	$SU_CMD 'curl -fsSL https://download.docker.com/linux/debian/gpg' | gpg --no-tty --dearmor -o /etc/apt/keyrings/docker.gpg 2>&1 >/dev/null
	chmod a+r /etc/apt/keyrings/docker.gpg
    fi
    if [ ! -e /etc/apt/sources.list.d/docker.list ]; then
	echo \
	    "deb [arch="$(dpkg --print-architecture)" signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/debian \
  "$(. /etc/os-release && echo "$VERSION_CODENAME")" stable" | \
	    tee /etc/apt/sources.list.d/docker.list > /dev/null

    fi
    apt-get update
    apt-get install $DEB_OPTIONS docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

    adduser vagrant docker >/dev/null

    if [ "$VB_GUI" == "y" ]; then
	apt-get install -y lxde
    fi

    ## Additional configuration to also support developing with Trip Server version 1 series.
    if [ "$TRIP_DEV" == "y" ]; then
	grep -E '^11\.' /etc/debian_version
	if [ $? -eq 0 ]; then
	    apt-get install -y openjdk-11-jdk chromium chromium-l10n firefox-esr-l10n-en-gb
	    install_nodejs
	else
	    apt-get install -y openjdk-17-jdk chromium chromium-l10n firefox-esr-l10n-en-gb
	    install_nodejs
	fi
    fi
fi
