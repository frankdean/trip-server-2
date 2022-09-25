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
DOWNLOAD_CACHE_DIR="/vagrant/provisioning/downloads"
install_asio="n"
install_boost="n"

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
apt-get install -y tzdata
dpkg-reconfigure tzdata

apt-get install $DEB_OPTIONS apt-transport-https
apt-get install $DEB_OPTIONS g++ git postgresql postgresql-contrib postgis \
	libpqxx-dev screen autoconf autoconf-doc automake autoconf-archive \
	libtool gettext valgrind uuid-dev uuid-runtime make nginx apg \
	libboost-locale-dev autopoint intltool gdb \
	libyaml-cpp-dev nlohmann-json3-dev \
	docbook2x texlive info

if [ "$VB_GUI" == "y" ]; then
    apt-get install -y lxde
fi

## Additional configuration to also support developing with Trip Server version 1 series.
if [ "$TRIP_DEV" == "y" ]; then
	apt-get install -y openjdk-11-jdk chromium chromium-l10n firefox-esr-l10n-en-gb vim
fi

NODE_VERSION="v14.20.1"
NODE_SHA256=2938c38c3654810f99101c9dc91e4783f14ed94eba878fef2906b6a9b95868a8
NODE_FILENAME="node-${NODE_VERSION}-linux-x64"
NODE_TAR_FILENAME="${NODE_FILENAME}.tar.xz"
NODE_EXTRACT_DIR="${NODE_FILENAME}"
NODE_DOWNLOAD_URL="https://nodejs.org/dist/${NODE_VERSION}/${NODE_TAR_FILENAME}"

if [ ! -d "/usr/local/share/${NODE_FILENAME}" ]; then
	if [ ! -e "/vagrant/provisioning/downloads/${NODE_TAR_FILENAME}" ]; then
		if [ ! -d /vagrant/provisioning/downloads ]; then
			mkdir -p /vagrant/provisioning/downloads
		fi
		cd /vagrant/provisioning/downloads
		wget --no-verbose $NODE_DOWNLOAD_URL 2>&1
		echo "$NODE_SHA256  $NODE_TAR_FILENAME" | shasum -c -
		if [ $? -ne "0" ]; then
			>&2 echo "Checksum of downloaded file does not match expected value of ${NODE_SHA256}"
			exit 1
		fi
	fi
	if [ -e "/vagrant/provisioning/downloads/${NODE_TAR_FILENAME}" ]; then
		cd /vagrant/provisioning/downloads
		echo "$NODE_SHA256  $NODE_TAR_FILENAME" | shasum -c -
		if [ $? -ne "0" ]; then
			>&2 echo "Checksum of downloaded file does not match expected value of ${NODE_SHA256}"
			exit 1
		fi
		cd /usr/local/share
		tar --no-same-owner --no-same-permissions -xf "/vagrant/provisioning/downloads/${NODE_TAR_FILENAME}"
		if [ -L /usr/local/share/node-current ]; then
		    rm -f /usr/local/share/node-current
		fi
		ln -sf "$NODE_EXTRACT_DIR" node-current
		cd  /usr/local/bin
		ln -sf ../share/node-current/bin/node
		ln -sf ../share/node-current/bin/npm
		ln -sf ../share/node-current/bin/npx
		cd /usr/local/include
		ln -sf ../share/node-current/include/node/
		cd /usr/local/lib
		ln -sf ../share/node-current/lib/node_modules/
		mkdir -p /usr/local/share/doc
		cd /usr/local/share/doc
		ln -sf ../node-current/share/doc/node/
		mkdir -p /usr/local/share/man/man1/
		cd /usr/local/share/man/man1/
		ln -sf ../../node-current/share/man/man1/node.1
		cd /usr/local/share/
		mkdir -p systemtap/tapset
		cd systemtap/tapset
		ln -sf ../../node-current/share/systemtap/tapset/node.stp
	fi
	if [ -e /vagrant/node_modules ]; then
		cd /vagrant
		rm -rf node_modules
	fi
fi
if [ ! -x /usr/local/share/node-current/lib/node_modules/yarn/bin/yarn ];then
   sudo npm install -g yarn
fi
if [ ! -h /usr/local/bin/yarn ]; then
    ln -s /usr/local/share/node-current/lib/node_modules/yarn/bin/yarn /usr/local/bin/yarn
fi
## END Additional configuration to also support developing with Trip Server version 1 series.
