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
NODE_VERSION="v14.20.1"
NODE_SHA256=2938c38c3654810f99101c9dc91e4783f14ed94eba878fef2906b6a9b95868a8

DOWNLOAD_CACHE_DIR="/vagrant/provisioning/downloads"
LIBPQXX_DOWNLOAD="libpqxx-${LIBPQXX_VERSION}.tar.gz"
LIBPQXX_DOWNLOAD_URL="https://github.com/jtv/libpqxx/archive/refs/tags/${LIBPQXX_VERSION}.tar.gz"
NLOHMANN_JSON_DOWNLOAD_URL="https://github.com/nlohmann/json/releases/download/${NLOHMANN_JSON_VERSION}/json.hpp"

SU_CMD="su vagrant -c"

function install_libpqxx_6
{
    # Download and install libpqxx if it does not exist
    if [ ! -d /usr/local/include/pqxx ]; then
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
		>&2 echo "Checksum of ${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD} does not match expected value of ${LIBPQXX_SHA256}"
		exit 1
	    fi
	    cd /usr/local/src
	    mkdir "libpqxx-${LIBPQXX_VERSION}"
	    chown vagrant:vagrant "libpqxx-${LIBPQXX_VERSION}"
	    $SU_CMD "tar -C /usr/local/src -xf ${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD}"
	    cd "libpqxx-${LIBPQXX_VERSION}"
	    pwd
	    if [ -r configure ]; then
		$SU_CMD './configure --disable-documentation'
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

function install_nodejs_14
{
    # https://github.com/nodejs/help/wiki/Installation
    NODE_FILENAME="node-${NODE_VERSION}-linux-x64"
    NODE_TAR_FILENAME="${NODE_FILENAME}.tar.xz"
    NODE_EXTRACT_DIR="${NODE_FILENAME}"
    NODE_DOWNLOAD_URL="https://nodejs.org/dist/${NODE_VERSION}/${NODE_TAR_FILENAME}"

    if [ ! -x /usr/local/lib/nodejs/node-current/bin/node ]; then
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
    egrep '^export\s+PATH.*nodejs' /home/vagrant/.profile >/dev/null 2>&1
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
# Fedora provisioning
#
##############################################################################

if [ -f /etc/rocky-release ] || [ -f /usr/lib/fedora-release ]; then
    DNF_OPTIONS="--assumeyes"
    dnf $DNF_OPTIONS install gcc gawk boost-devel libpq-devel \
	libpq-devel yaml-cpp-devel pugixml-devel libuuid-devel curl \
	postgresql-server postgresql-contrib \
	screen vim autoconf automake info libtool \
	intltool gdb valgrind git apg \
	nodejs

    if [ -f /etc/rocky-release ]; then
	# Rocky Linux fails to install the VirtualBox Guest Additions kernel with:
	#
	# > Could not find the X.Org or XFree86 Window System, skipping.
	# > modprobe vboxguest failed
	#
	# Installed xorg-x11-server-Xorg manually - do we need to install it?

	# rockylinux: Error: Unable to find a match: postgis autoconf-archive texinfo texinfo-tex yarnpkg

	dnf $DNF_OPTIONS update
	dnf $DNF_OPTIONS install epel-release
	dnf $DNF_OPTIONS install gcc gcc-c++ make perl kernel-devel kernel-headers \
	    bzip2 dkms boost
	dnf $DNF_OPTIONS update 'kernel-*'

	# The Rocky Linux box otherwise uninstalls this package with 'dnf autoremove'
	dnf $DNF_OPTIONS mark install NetworkManager-initscripts-updown \
	    grub2-tools-efi python3-configobj rdma-core

    elif [ -f /usr/lib/fedora-release ]; then
	# The Fedora box otherwise uninstalls this package with 'dnf autoremove'
	dnf $DNF_OPTIONS mark install linux-firmware-whence
	dnf $DNF_OPTIONS install postgis autoconf-archive texinfo texinfo-tex \
	    yarnpkg
    fi

    install_libpqxx_6
    install_nlohmann_json
    if [ "$VB_GUI" == "y" ]; then
	dnf $DNF_OPTIONS group install lxde-desktop
    fi

    if [ ! -r /var/lib/pgsql/data/postgresql.conf ]; then
	postgresql-setup --initdb
    fi
    systemctl is-active postgresql.service >/dev/null
    if [ $? -ne 0 ]; then
	systemctl enable postgresql.service
	systemctl start postgresql.service
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
	intltool gdb libtool autoconf-archive gettext automake \
	node14 yarn-node14
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
	    libyaml-cpp-dev nlohmann-json3-dev \
	    docbook2x texlive info texinfo curl

    if [ "$VB_GUI" == "y" ]; then
	apt-get install -y lxde
    fi

    ## Additional configuration to also support developing with Trip Server version 1 series.
    if [ "$TRIP_DEV" == "y" ]; then
	apt-get install -y openjdk-11-jdk chromium chromium-l10n firefox-esr-l10n-en-gb vim
	install_nodejs_14
    fi
    ## END Additional configuration to also support developing with Trip Server version 1 series.
fi
