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

SU_CMD="su vagrant -c"

if [ -x /bin/freebsd-version ]; then
    SU_CMD="su -l vagrant -c"

    grep -E '^\s*:lang=en_GB.UTF-8' /home/vagrant/.login_conf >/dev/null 2>&1
    if [ $? -ne 0 ]; then
	cat >> /home/vagrant/.login_conf <<"EOF" >/dev/null 2>&1
me:\
	:charset=ISO8859-15:\
	:lang=en_GB.UTF-8:
EOF
    fi
fi

# Create trip-server configuration file from distribution file
# signingKey and resourceSigningKey only needed for trip v1
SIGNING_KEY=$(apg  -m 12 -x 14 -M NC -t -n 20 | tail -n 1 | cut -d ' ' -f 1 -)
if [ $? -ne 0 ]; then
    echo "apg is not installed"
    exit 1
fi

# If key is absent from config, suggests a previous failure to create
grep -E '^\s+signingKey' /usr/local/etc/trip-server.yaml >/dev/null 2>&1
if [ $? -ne 0 ] || [ -L /usr/local/etc/trip-server.yaml ]; then
    rm -f /usr/local/etc/trip-server.yaml
fi
if [ ! -e /usr/local/etc/trip-server.yaml ]; then
    if [ -x /bin/freebsd-version ]; then
	# FreeBSD different location for PostgreSQL socket
	sed "s/level: 0/level: 4/; s/signingKey.*/signingKey: ${SIGNING_KEY}/; s/resourceSigningKey.*/resourceSigningKey: ${SIGNING_KEY}/; s/uri: .*/uri: postgresql:\/\/%2Ftmp\/trip/;" /vagrant/conf/trip-server-dist.yaml >/usr/local/etc/trip-server.yaml
    else
	sed "s/level: 0/level: 4/; s/signingKey.*/signingKey: ${SIGNING_KEY}/; s/resourceSigningKey.*/resourceSigningKey: ${SIGNING_KEY}/; s/uri: .*/uri: postgresql:\/\/%2Fvar%2Frun%2Fpostgresql\/trip/;" /vagrant/conf/trip-server-dist.yaml >/usr/local/etc/trip-server.yaml
    fi
fi

# Configure PostgeSQL
su - postgres -c 'createuser -drs vagrant' 2>/dev/null
cd /vagrant

if [ -d "/etc/postgresql/${PG_VERSION}" ]; then
	grep -E 'local\s+trip\s+trip\s+md5' "/etc/postgresql/${PG_VERSION}/main/pg_hba.conf" >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		sed -i 's/local\(.*all.*all.*$\)/local   trip          trip                                  md5\nlocal\1/' "/etc/postgresql/${PG_VERSION}/main/pg_hba.conf"
		systemctl reload postgresql
	fi
fi
if [ "$WIPE_DB" == "y" ]; then
	su - postgres -c 'dropdb trip' 2>/dev/null
	su - postgres -c 'dropuser trip' 2>/dev/null
fi

TEST_DATA_DIR=/vagrant/provisioning/schema
if [ -d "$TEST_DATA_DIR" ]; then
    su - postgres -c 'createuser trip' >/dev/null 2>&1
    if [ $? -eq 0 ]; then
	su - postgres -c 'dropuser trip'
	echo "CREATE USER trip NOSUPERUSER NOCREATEDB NOCREATEROLE INHERIT" 2>/dev/null 2>&1 | su - postgres -c 'psql --quiet' 2>/dev/null 2>&1
    fi
    su - postgres -c 'createdb trip --owner=trip' 2>/dev/null
    if [ $? -eq 0 ]; then
	su - postgres -c 'psql trip' <<EOF >/dev/null 2>&1
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
	su - postgres -c 'psql trip' <<EOF >/dev/null 2>&1
CREATE EXTENSION pgcrypto;
EOF
	# Create Test Data
	if [ -r "$DB_TEST_DATA" ]; then
            su - postgres -c  'psql trip' <"$DB_TEST_DATA" >/dev/null
	fi
    fi
fi
# Setup nginx as an example
if [ -d /etc/nginx ]; then
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
fi

# Build the application
if [ ! -d /home/vagrant/build ]; then
    $SU_CMD 'mkdir /home/vagrant/build'
fi
cd /home/vagrant/build
if [ ! -d /home/vagrant/build/provisioning ]; then
    $SU_CMD 'cp -a /vagrant/provisioning .'
fi
# Don't build if we appear to already have an installed version of trip-server
if [ ! -x /usr/local/bin/trip-server ]; then
    if [ -r /usr/lib/fedora-release ] || [ -x /bin/freebsd-version ]; then
	$SU_CMD "pwd && /vagrant/configure PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$(pg_config --libdir)/pkgconfig CXXFLAGS='-g -O2' --disable-gdal"
    elif [ -f /etc/rocky-release ]; then
	# Weirdly, pkg-config was appending a spurious '-L' with the '--libs'
	# parameter for 'libpqxx', proven with the following command:
	# PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/pgsql-13/lib/pkgconfig pkg-config libpqxx --libs
	# So we override pkg-config...
	PG_LIBDIR=$(/usr/pgsql-13/bin/pg_config --libdir)
	# SUCCESS -> $SU_CMD "pwd && /vagrant/configure LIBPQXX_LIBS="'"-lpqxx -lpq"'" LDFLAGS=-L${PG_LIBDIR} PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:${PG_LIBDIR}/pkgconfig CXXFLAGS='-g -O2' --disable-gdal"
	# SUCCESS -> $SU_CMD "pwd && /vagrant/configure LIBPQXX_LIBS="'"-lpqxx -lpq"'" LDFLAGS=-L${PG_LIBDIR} PKG_CONFIG_PATH=/usr/lib64/pkgconfig:/usr/share/pkgconfig:/usr/local/lib/pkgconfig:${PG_LIBDIR}/pkgconfig CXXFLAGS='-g -O2' --disable-gdal"
	$SU_CMD "pwd && /vagrant/configure LIBPQXX_LIBS="'"-lpqxx -lpq"'" LDFLAGS=-L${PG_LIBDIR} PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:${PG_LIBDIR}/pkgconfig CXXFLAGS='-g -O2' --disable-gdal"
    else
	$SU_CMD "/vagrant/configure CXXFLAGS='-g -O2' --disable-gdal"
    fi

    $SU_CMD 'pwd && make -C /home/vagrant/build check'
    if [ $? -eq 0 ] && [ -x /home/vagrant/build/src/trip-server ]; then
	echo "Installing trip-server"
	make install
    fi
fi

if [ -x /home/vagrant/build/src/trip-server ]; then
    RESULT=$($SU_CMD "echo 'SELECT count(*) AS session_table_exists FROM session' | psql trip 2>&1") >/dev/null 2>&1
    echo $RESULT | grep -E 'ERROR:\s+relation "session" does not exist' >/dev/null 2>&1
    if [ $? -eq 0 ]; then
	echo "Upgrading database"
	$SU_CMD '/usr/local/bin/trip-server --upgrade'
    fi
fi

# Vi as default editor
grep -E '^export\s+EDITOR' /home/vagrant/.profile >/dev/null 2>&1
if [ $? -ne 0 ]; then
	echo "export EDITOR=/usr/bin/vi" >>/home/vagrant/.profile
fi

# Configure systemd if it appears to be installed
if [ -d /etc/systemd/system ] && [ ! -e /etc/systemd/system/trip-server-2.service ]; then
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
if [ ! -e /etc/logrotate.d/trip ] && [ -d /etclogrotate.d ]; then
    cp /vagrant/provisioning/logrotate.d/trip /etc/logrotate.d/
fi

## Additional configuration to support developing with Trip Server version 1 series.
if [ -d /vagrant-trip-server ]; then
    cd /vagrant-trip-server
    if [ ! -e config.yaml ]; then
	if [ -f /usr/local/etc/trip-server.yaml ]; then
	    $SU_CMD 'ln -s /usr/local/etc/trip-server.yaml config.yaml'
	else
	    UMASK=$(umask -p)
	    umask o-rwx
	    cp config-dist.yaml config.yaml
	    $UMASK
	    sed "s/level: 0/level: 4/; s/signingKey.*/signingKey: ${SIGNING_KEY},/; s/maxAge: *[0-9]\+/maxAge: 999/; s/host: *.*, */host: localhost,/; s/^        path: .*$/        path: \/DO_NOT_FETCH_TILES_IN_DEMO_UNTIL_PROPERLY_CONFIGURED\/{z}\/{x}\/{y}.png,/; s/uri: .*/uri: postgresql:\/\/%2Fvar%2Frun%2Fpostgresql\/trip/; s/allow: +.*/allow: false/; s/level: +info/level: debug/" config-dist.yaml >config.yaml
	fi
    fi
fi

# Remove any existing link and then re-create
if [ -L /vagrant-trip-server/app ]; then
	rm /vagrant-trip-server/app
fi

# Our normal setup for trip-server v1 doesn't work with the folder
# synchronisation used for the FreeBSD box
if [ ! -x /bin/freebsd-version ]; then
    if [ -f /vagrant-trip-server/package.json ] && [ ! -d /vagrant-trip-server/node_modules ]; then
	$SU_CMD 'cd /vagrant-trip-server && PATH=/usr/local/lib/nodejs/node-current/bin:$PATH yarn install'
    fi
    if [ -f /vagrant-trip-web-client/package.json ]; then
	echo "Configuring web client to use shared folder under /vagrant-trip-web-client/"
	if [ ! -d /vagrant-trip-web-client/node_modules ]; then
	    $SU_CMD 'cd /vagrant-trip-web-client && PATH=/usr/local/lib/nodejs/node-current/bin:$PATH yarn install'
	fi
	if [ "$TRIP_DEV" == "y" ]; then
	    if [ -L /vagrant-trip-server/app ]; then
		rm /vagrant-trip-server/app
	    fi
	    ln -f -s /vagrant-trip-web-client/app /vagrant-trip-server/app
	fi
    fi
fi
if [ "$TRIP_DEV" == "y" ]; then
	grep -E '^export\s+CHROME_BIN' /home/vagrant/.profile >/dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "export CHROME_BIN=/usr/bin/chromium" >>/home/vagrant/.profile
	fi
fi
## END Additional configuration to also support developing with Trip Server version 1 series.
