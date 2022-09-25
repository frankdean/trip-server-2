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

##Debian 10
#PG_VERSION=11
# Debian 11
PG_VERSION=13
PGPASSWORD='syipsixdod2'

# Configure PostgeSQL
su - postgres -c 'createuser -drs vagrant' 2>/dev/null
cd /vagrant

if [ -d "/etc/postgresql/${PG_VERSION}" ]; then
	egrep 'local\s+trip\s+trip\s+md5' "/etc/postgresql/${PG_VERSION}/main/pg_hba.conf" >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		sed -i 's/local\(.*all.*all.*$\)/local   trip          trip                                  md5\nlocal\1/' "/etc/postgresql/${PG_VERSION}/main/pg_hba.conf"
		systemctl reload postgresql
	fi
fi
if [ "$WIPE_DB" == "y" ]; then
	su - postgres -c 'dropdb trip' 2>/dev/null
	su - postgres -c 'dropuser trip' 2>/dev/null
fi
su - postgres -c 'createuser trip' >/dev/null 2>&1
if [ $? -eq 0 ]; then
	su - postgres -c 'dropuser trip'
	echo "CREATE USER trip PASSWORD '${PGPASSWORD}' NOSUPERUSER NOCREATEDB NOCREATEROLE INHERIT" 2>/dev/null 2>&1 | su - postgres -c 'psql --quiet' 2>/dev/null 2>&1
fi
TEST_DATA_DIR=/vagrant/provisioning/downloads
su - postgres -c 'createdb trip --owner=trip' 2>/dev/null
if [ $? -eq 0 ]; then
	su - postgres -c 'psql trip' >/dev/null 2>&1 <<EOF
CREATE ROLE trip_role;
GRANT trip_role TO trip;
EOF
	DB_SCHEMA="${TEST_DATA_DIR}/20_schema.sql"
	DB_PERMS="${TEST_DATA_DIR}/30_permissions.sql"
	if [ -r "$DB_SCHEMA" ]; then
	    su - postgres -c 'psql trip' <"${DB_SCHEMA}" >/dev/null
	    CREATED_DB='y'
	fi
	if [ -r "$DB_PERMS" ]; then
	    su - postgres -c  'psql trip' <"${DB_PERMS}" >/dev/null
	fi
fi
if [ "$CREATED_DB" == "y" ]; then
    DB_TEST_DATA="${TEST_DATA_DIR}/90_test-data.sql"
    su - postgres -c 'psql trip' >/dev/null <<EOF
CREATE EXTENSION pgcrypto;
EOF
    # Create Test Data
    if [ -r "$DB_TEST_DATA" ]; then
        su - postgres -c  'psql trip' <"$DB_TEST_DATA" >/dev/null
    fi
fi
# Setup nginx as an example
if [ ! -e /etc/nginx/conf.d/trip.conf ]; then
	cp /vagrant/provisioning/nginx/conf.d/trip.conf /etc/nginx/conf.d/
fi
if [ ! -e /etc/nginx/sites-available/trip ]; then
	cp /vagrant/provisioning/nginx/sites-available/trip /etc/nginx/sites-available
fi
cd /etc/nginx/sites-enabled
if [ -e default ]; then
	rm -f default
	ln -fs ../sites-available/trip
fi
systemctl restart nginx

## Additional configuration to support developing with Trip Server version 1 series.
if [ -d /vagrant-trip-server ]; then
    cd /vagrant-trip-server
    if [ ! -e config.yaml ]; then
	SIGNING_KEY=$(apg  -m 12 -x 14 -M NC -t -n 20 | tail -n 1 | cut -d ' ' -f 1 -)
	UMASK=$(umask -p)
	umask o-rwx
	cp config-dist.yaml config.yaml
	$UMASK
	sed "s/level: 0/level: 4/; s/signingKey.*/signingKey: ${SIGNING_KEY},/; s/maxAge: *[0-9]\+/maxAge: 999/; s/host: *.*, */host: localhost,/; s/^        path: .*$/        path: \/DO_NOT_FETCH_TILES_IN_DEMO_UNTIL_PROPERLY_CONFIGURED\/{z}\/{x}\/{y}.png,/; s/uri: .*/uri: postgresql:\/\/%2Fvar%2Frun%2Fpostgresql\/trip/; s/allow: +.*/allow: false/; s/level: +info/level: debug/" config-dist.yaml >config.yaml
    fi
    if [ ! -L /usr/local/etc/trip-server.yaml ]; then
	ln -s /vagrant-trip-server/config.yaml /usr/local/etc/trip-server.yaml
    fi
fi

# Build the application
if [ ! -d /home/vagrant/build ]; then
    su vagrant -c 'mkdir /home/vagrant/build'
fi
cd /home/vagrant/build
su - vagrant -c 'cd /home/vagrant/build && /vagrant/configure'
su - vagrant -c 'make -C /home/vagrant/build check'
if [ -x /home/vagrant/build/src/trip-server ]; then
    echo "Installing trip-server"
    make install
    echo "Upgrade?"
    if [ "$CREATED_DB" == "y" ]; then
	echo "Upgrading database"
	su - vagrant -c 'trip-server --upgrade'
    fi
fi
# Vi as default editor
egrep '^export\s+EDITOR' /home/vagrant/.profile >/dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "export EDITOR=/usr/bin/vi" >>/home/vagrant/.profile
fi
# Configure systemd
if [ ! -e /etc/systemd/system/trip-server-2.service ]; then
    systemctl is-active trip-server-2.service >/dev/null
    if [ $? -ne 0 ]; then
	cp /vagrant/provisioning/systemd/trip-server-2.service /etc/systemd/system/
	# systemctl enable trip-server-2.service 2>/dev/null
	# systemctl start trip-server-2.service 2>/dev/null
    fi
fi
if [ ! -e /var/log/trip.log ]; then
    touch /var/log/trip.log
fi
chown vagrant:vagrant /var/log/trip.log
chmod 0640 /var/log/trip.log
if [ ! -e /etc/logrotate.d/trip ]; then
    cp /vagrant/provisioning/logrotate.d/trip /etc/logrotate.d/
fi

# Remove any existing link and then re-create
if [ -L /vagrant-trip-server/app ]; then
	rm /vagrant-trip-server/app
fi
if [ -f /vagrant-trip-web-client/package.json ]; then
	echo "Configuring web client to use shared folder under /vagrant-trip-web-client/"
	if [ ! -d /vagrant-trip-web-client/node_modules ]; then
		su - vagrant -c 'cd /vagrant-trip-web-client && yarn install'
	fi
	if [ "$TRIP_DEV" == "y" ]; then
		if [ -L /vagrant-trip-server/app ]; then
			rm /vagrant-trip-server/app
		fi
		ln -f -s /vagrant-trip-web-client/app /vagrant-trip-server/app
	fi
fi
if [ "$TRIP_DEV" == "y" ]; then
	egrep '^export\s+CHROME_BIN' /home/vagrant/.profile >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "export CHROME_BIN=/usr/bin/chromium" >>/home/vagrant/.profile
	fi
fi
## END Additional configuration to also support developing with Trip Server version 1 series.
