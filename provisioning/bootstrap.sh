#!/usr/bin/env bash

# This file is part of Trip Server 2, a program to support trip recording and
# itinerary planning.
#
# Copyright (C) 2022-2025 Frank Dean <frank.dean@fdsd.co.uk>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Affero General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
# for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Whilst this script is primarily intended as a Vagrant provisioning script,
# it can also be run in a VM such as QEMU to setup up a working environment by
# overriding the TRIP_SOURCE, USERNAME and GROUPNAME variables before running
# the script.

# Uncomment the following to debug the script
#set -x

# This is the folder where the Trip Server source code is available in the VM
TRIP_SOURCE=${TRIP_SOURCE:-/vagrant}
# The user and group name for building Trip Server
USERNAME=${USERNAME:-vagrant}
GROUPNAME=${GROUPNAME:-${USERNAME}}
# Flags to pass to build, e.g. '--jobs 4'
MAKEFLAGS=${MAKEFLAGS:-}

echo "Source folder: ${TRIP_SOURCE}"
echo "Username: ${USERNAME}"
echo "Groupname: ${USERNAME}:${GROUPNAME}"
echo "MAKEFLAGS: ${MAKEFLAGS}"

export MAKEFLAGS

# This version is only relevant if libpqxx 6.x is being installed.  Trip
# Server supports either version 6 or 7.
LIBPQXX_VERSION=6.4.8
LIBPQXX_SHA256=3f7aba951822e01f1b9f9f353702954773323dd9f9dc376ffb57cb6bbd9a7a2f
NLOHMANN_JSON_VERSION=v3.11.3
NLOHMANN_JSON_SHA256=9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6
FINALCUT_VERSION=0.9.0
FINALCUT_SHA256=73ff5016bf6de0a5d3d6e88104668b78a521c34229e7ca0c6a04b5d79ecf666e

DOWNLOAD_CACHE_DIR="${TRIP_SOURCE}/provisioning/downloads"
LIBPQXX_DOWNLOAD="libpqxx-${LIBPQXX_VERSION}.tar.gz"
LIBPQXX_DOWNLOAD_URL="https://github.com/jtv/libpqxx/archive/refs/tags/${LIBPQXX_VERSION}.tar.gz"
NLOHMANN_JSON_DOWNLOAD_URL="https://github.com/nlohmann/json/releases/download/${NLOHMANN_JSON_VERSION}/json.hpp"
FINALCUT_DOWNLOAD="finalcut-0.9.0.tar.gz"
FINALCUT_DOWNLOAD_URL="https://github.com/gansm/finalcut/archive/refs/tags/${FINALCUT_VERSION}.tar.gz"

SHASUM=shasum
SU_CMD="su $USERNAME -c"

if [ -r /usr/lib/os-release ]; then
    source /usr/lib/os-release
elif [ -r /var/run/os-release ]; then
    source /var/run/os-release
fi

echo "ID: $ID"
echo "$PRETTY_NAME"

if [ -z $ID ];then
    echo "Unable to determine OS"
    exit 1;
fi

if [ "$ID" == "fedora" ] || [ "$ID" == "rocky" ]; then
    SHASUM=sha256sum
fi
if [ ! -d "$DOWNLOAD_CACHE_DIR" ] || [ "$ID" == "freebsd" ]; then
    # Where the TRIP_SOURCE folder is mounted with 9p, whatever we do, from
    # the host perspective, if we created the directory is is owned by root,
    # even when we change the ownership from within the guest.  So, use a
    # local folder for the downloads.
    #
    # Also the provisioning folder isn't synced on FreeBSD
    DOWNLOAD_CACHE_DIR="/home/${USERNAME}/Downloads"
    if [ ! -d "$DOWNLOAD_CACHE_DIR" ]; then
	$SU_CMD "mkdir -p $DOWNLOAD_CACHE_DIR"
    fi
    echo "Using $DOWNLOAD_CACHE_DIR to cache downloads"
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
		$SU_CMD "curl -L --output ${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD} $LIBPQXX_DOWNLOAD_URL"
	    fi
	fi
	if [ -r "${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD}" ]; then
	    echo "$LIBPQXX_SHA256  ${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD}" | $SHASUM -c -
	    if [ $? -ne "0" ]; then
		2>&1 echo "Checksum of ${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD} does not match expected value of ${LIBPQXX_SHA256}"
		exit 1
	    fi
	fi
	if [ ! -f /usr/local/include/pqxx/pqxx ]; then
	    cd /usr/local/src
	    if [ ! -d "libpqxx-${LIBPQXX_VERSION}" ]; then
		mkdir "libpqxx-${LIBPQXX_VERSION}"
	    fi
	    chown ${USERNAME}:${GROUPNAME} libpqxx-${LIBPQXX_VERSION}
	    $SU_CMD "tar -C /usr/local/src -xf ${DOWNLOAD_CACHE_DIR}/${LIBPQXX_DOWNLOAD}"
	    cd "libpqxx-${LIBPQXX_VERSION}"
	    pwd
	    if [ -r configure ]; then
		# configure command fails on VM running Debian arm64
		PROCESSOR="$(uname -p)"
		ARCH="$(arch)"
		if [ "$PROCESSOR" -!= "unknown" ]; then
		       $SU_CMD './configure --disable-documentation'
		elif [ "$ARCH" == "aarch64" ]; then
		    $SU_CMD "./configure --disable-documentation --build=arm"
		else
		    $SU_CMD "./configure --disable-documentation --build=$ARCH"
		fi
		$SU_CMD "time make"
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
	    # https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
	    $SU_CMD "curl -L --output ${DOWNLOAD_CACHE_DIR}/nlohmann-json-${NLOHMANN_JSON_VERSION}.hpp $NLOHMANN_JSON_DOWNLOAD_URL"
	fi
	if [ -r "${DOWNLOAD_CACHE_DIR}/nlohmann-json-${NLOHMANN_JSON_VERSION}.hpp" ]; then
	    echo "$NLOHMANN_JSON_SHA256  ${DOWNLOAD_CACHE_DIR}/nlohmann-json-${NLOHMANN_JSON_VERSION}.hpp" | $SHASUM -c -
	    if [ $? -ne "0" ]; then
		>&2 echo "Checksum of ${DOWNLOAD_CACHE_DIR}/nlohmann-json-${NLOHMANN_JSON_VERSION}.hpp does not match expected value of ${NLOHMANN_JSON_SHA256}"
		exit 1
	    fi
	    mkdir /usr/local/include/nlohmann
	    cp "${DOWNLOAD_CACHE_DIR}/nlohmann-json-${NLOHMANN_JSON_VERSION}.hpp" /usr/local/include/nlohmann/json.hpp
	    chmod go+r /usr/local/include/nlohmann/json.hpp
	fi
    fi
}

function install_finalcut
{
    if [ ! -r /usr/local/include/final/final.h ]; then
	if [ ! -d "/usr/local/src/finalcut-${FINALCUT_VERSION}" ]; then
	    if [ ! -r "${DOWNLOAD_CACHE_DIR}/finalcut-${FINALCUT_VERSION}.tar.gz" ]; then
		$SU_CMD "curl -L --output ${DOWNLOAD_CACHE_DIR}/${FINALCUT_DOWNLOAD} $FINALCUT_DOWNLOAD_URL"
	    fi
	    if [ ! -r "${DOWNLOAD_CACHE_DIR}/finalcut-${FINALCUT_VERSION}.tar.gz" ]; then
		echo "$FINALCUT_SHA256  ${DOWNLOAD_CACHE_DIR}/${FINALCUT_DOWNLOAD}" | $SHASUM -c -
		if [ $? -ne "0" ]; then
		    2>&1 echo "Checksum of ${DOWNLOAD_CACHE_DIR}/${FINALCUT_DOWNLOAD} does not match expected value of ${FINALCUT_SHA256}"
		    exit 1
		fi
	    fi
	fi

	if [ ! -r /usr/local/include/final/final.h ]; then
	    if [ ! -d /usr/local/src ]; then
		mkdir /usr/local/src
	    fi
	    cd /usr/local/src
	    if [ ! -d "finalcut-${FINALCUT_VERSION}" ]; then
		mkdir "finalcut-${FINALCUT_VERSION}"
	    fi
	    chown ${USERNAME}:${GROUPNAME} finalcut-${FINALCUT_VERSION}
	    $SU_CMD "tar -C /usr/local/src -xf ${DOWNLOAD_CACHE_DIR}/${FINALCUT_DOWNLOAD}"
	    cd "finalcut-${FINALCUT_VERSION}"
	    pwd
	    $SU_CMD "autoreconf --install"
	    if [ "$ID" == "freebsd" ]; then
		$SU_CMD "./configure PKG_CONFIG_PATH=/usr/local/lib/pkgconfig"
	    else
		$SU_CMD "./configure PKG_CONFIG_PATH=/usr/local/lib/pkgconfig CXXFLAGS=-Wno-dangling-reference"
	    fi
	    $SU_CMD "time make"
	    make install
	fi
    fi
}

##############################################################################
#
# Fedora & Rocky Linux provisioning
#
##############################################################################

if [ "$ID" == "rocky" ] || [ "$ID" == "fedora" ]; then
    DNF_OPTIONS="--assumeyes"

    if [ "$ID" == "rocky" ]; then
	# Rocky Linux fails to install the VirtualBox Guest Additions kernel with:
	#
	# > Could not find the X.Org or XFree86 Window System, skipping.
	# > modprobe vboxguest failed
	#
	# Installed xorg-x11-server-Xorg manually - do we need to install it?

	dnf $DNF_OPTIONS update --refresh
	dnf $DNF_OPTIONS install epel-release
	# https://forums.rockylinux.org/t/how-do-i-install-powertools-on-rocky-linux-9/7427/3
	dnf $DNF_OPTIONS config-manager --enable crb
	# for Rocky 8 and above might need to do this as well
	crb enable
    fi

    dnf $DNF_OPTIONS install gcc gcc-c++  \
	libuuid-devel uuidd gpm-devel ncurses-devel \
	curl libX11 libXt libXmu \
	autoconf-archive texinfo texinfo-tex yarnpkg \
	vim autoconf automake info libtool \
	intltool gdb valgrind git \
	yaml-cpp-devel pugixml-devel screen cmark-devel \
	bc apg openssl-devel

    if [ "$ID" == "rocky" ]; then
	# postgresql & postgis - https://postgis.net/install/
	# https://www.postgresql.org/download/linux/redhat/
	# --disablerepo=* fixes Error: Failed to download metadata for repo 'pgdg-common': repomd.xml GPG signature verification error: Signing key not found
	# which seems to stop all dnf commands from working.
	# https://yum.postgresql.org/news/pgdg-rpm-repo-gpg-key-update/
	ARCH="$(arch)"
	if [ "$ARCH" == "aarch64" ]; then
	    dnf --disablerepo=* $DNF_OPTIONS install https://download.postgresql.org/pub/repos/yum/reporpms/EL-9-aarch64/pgdg-redhat-repo-latest.noarch.rpm
	else
	    dnf --disablerepo=* $DNF_OPTIONS install https://download.postgresql.org/pub/repos/yum/reporpms/EL-9-x86_64/pgdg-redhat-repo-latest.noarch.rpm
	fi

	# Disable the built-in PostgreSQL module:
	#dnf $DNF_OPTIONS module disable postgresql
	dnf $DNF_OPTIONS install postgresql16-devel postgis34_16
	# Note that Rocky Linux box uninstalls some of these packages if 'dnf
	# autoremove' is run, so we mark them as explicitly installed.
	dnf $DNF_OPTIONS mark install NetworkManager-initscripts-updown \
	    grub2-tools-efi python3-configobj rdma-core \
	    make perl kernel-devel kernel-headers \
	    bzip2 dkms boost-locale
	dnf $DNF_OPTIONS update 'kernel-*'
	if [ $(echo "$VERSION_ID >= 10.0" |bc -l) -eq 1 ]; then
	    dnf $DNF_OPTIONS install cairomm1.16-devel
	else
	    dnf $DNF_OPTIONS install cairomm-devel
	fi

    elif [ "$ID" == "fedora" ]; then
	dnf $DNF_OPTIONS install cairomm-devel
	# The Fedora box otherwise uninstalls this package with 'dnf autoremove'
	dnf $DNF_OPTIONS mark install linux-firmware-whence gawk
	dnf $DNF_OPTIONS install postgresql-server postgresql-contrib postgis \
	    gdal-devel
    fi

    # Handling of config-manager package updates appears to happen in the
    # background so when we immediately go to install them they are apparently
    # not available.  By installing these packages at this later point, they
    # are much more likely to be available.
    dnf $DNF_OPTIONS install boost-devel libpq-devel libpqxx-devel

    #install_libpqxx_6
    install_finalcut
    install_nlohmann_json
    ldconfig /usr/local/lib
    if [ "$VB_GUI" == "y" ]; then
	dnf $DNF_OPTIONS group install lxde-desktop
    fi

    if [ "$ID" == "rocky" ]; then
	if [ ! -r /var/lib/pgsql/16/data/postgresql.conf ]; then
	    /usr/pgsql-16/bin/postgresql-16-setup initdb
	fi
	systemctl is-active postgresql-16 >/dev/null
	if [ $? -ne 0 ]; then
	    systemctl enable postgresql-16
	    systemctl start postgresql-16
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
    useradd --home-dir /nonexistent --system trip 2>/dev/null
fi

##############################################################################
#
# FreeBSD provisioning
#
##############################################################################
if [ "$ID" == "freebsd" ]; then
    FREEBSD_POSTGRESQL_VERSION=16
    FREEBSD_PKG_OPTIONS='--yes'
    pkg install $FREEBSD_PKG_OPTIONS pkgconf git boost-all yaml-cpp \
	postgresql${FREEBSD_POSTGRESQL_VERSION}-client \
	postgresql${FREEBSD_POSTGRESQL_VERSION}-contrib \
	postgresql${FREEBSD_POSTGRESQL_VERSION}-server \
	postgresql-libpqxx \
	postgis33 \
	python3 pugixml e2fsprogs-libuuid nlohmann-json \
	texinfo vim python3 apg \
	intltool gdb libtool autoconf-archive gettext automake cairomm \
	cmark
    # Include the textlive-full package to allow building the PDF docs, which
    # needs an additional 11G or more of disk space.
    
    # 8.7G used before adding these (7.8G used after clearing cache).
    #texlive-full

    # Remove the downloaded packages after installation
    #rm -f /var/cache/pkg/*

    install_finalcut
    if [ "$VB_GUI" == "y" ]; then
	pkg install $FREEBSD_PKG_OPTIONS lxde-meta
    fi
    chsh -s /usr/local/bin/bash ${USERNAME}

    if [ ! -d /var/db/postgres/data${FREEBSD_POSTGRESQL_VERSION} ]; then
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
    # adduser trip --system --group --home /nonexistent --no-create-home --quiet
    pw useradd -n trip -c 'trip daemon' -d /nonexistent -s /usr/sbin/nologin -w no 2>/dev/null
    # pw groupadd -n trip -M trip -q
fi

##############################################################################
#
# Debian/Ubuntu provisioning
#
##############################################################################
if [ "debian" == "$ID" ] || [ "ubuntu" == "$ID" ]; then
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
	    vim libncurses-dev libgpm-dev

    if [ ! -d /etc/apt/keyrings ]; then
	install -m 0755 -d /etc/apt/keyrings
    fi

    if [ "$VB_GUI" == "y" ]; then
	apt-get install -y lxde
    fi

    install_finalcut
    ldconfig

    adduser trip --system --group --home /nonexistent --no-create-home --quiet
fi

# Vi as default editor
if [ -f /home/${USERNAME}/.bash_profile ]; then
    grep -E '^export\s+EDITOR' /home/${USERNAME}/.bash_profile >/dev/null 2>&1
    if [ $? -ne 0 ]; then
	echo "export EDITOR=/usr/bin/vi" >>/home/${USERNAME}/.bash_profile
    fi
else
    grep -E '^export\s+EDITOR' /home/${USERNAME}/.profile >/dev/null 2>&1
    if [ $? -ne 0 ]; then
	echo "export EDITOR=/usr/bin/vi" >>/home/${USERNAME}/.profile
    fi
fi
if [ ! -z "$MAKEFLAGS" ]; then
    grep -E '^export\s+MAKEFLAGS' /home/${USERNAME}/.profile >/dev/null 2>&1
    if [ $? -ne 0 ]; then
	echo "export MAKEFLAGS='${MAKEFLAGS}'" >>/home/${USERNAME}/.profile
    fi
fi
