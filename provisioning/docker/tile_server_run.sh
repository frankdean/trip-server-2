#!/bin/bash

set -euo pipefail

import_database() {
    # Skip import if database tables already exist
    echo "Checking if external_data table exists in $POSTGRES_DB"
    result=$(sudo -u postgres psql --dbname $POSTGRES_DB --quiet --tuples-only --username $POSTGRES_USER --command="SELECT EXISTS (SELECT * FROM pg_tables WHERE schemaname='public' AND tablename='external_data')")
    success=$?
    if [ $success -eq 0 ] && [ "$result" == " t" ]; then
	echo "$POSTGRES_DB external_data table exists.  Skipping import."
    else
	echo "$POSTGRES_DB external_data table does not exist"
	echo "Importing database..."
	sed -i -e 's/max_wal_size = 1GB/max_wal_size = 2GB/' $PGDATA/postgresql.conf
	sudo -u postgres /usr/lib/postgresql/15/bin/pg_ctl reload -D "$PGDATA"
	sudo -u postgres pg_restore -d $POSTGRES_DB --no-owner --username=$POSTGRES_USER /downloads/gis.dmp
    fi
}

wait_until_db_ready() {
    is_ready=99
    until [ $is_ready -eq 0 ]
    do
	# The Postgres docker-entrypoint.sh script should still be running, if not, abort.
	is_ok="$(ps -p $pg_pid)"
	if [ -n "$is_ok" ]; then
	    if sudo -u postgres pg_isready --dbname $POSTGRES_DB --quiet --username $POSTGRES_USER; then
		is_ready=0
	    else
		is_ready=$?
	    fi
	    if [ $is_ready -eq 0 ] && [ ! -e "${PGDATA}/db-setup-complete" ]; then
		is_ready=1
	    fi
	    if [ $is_ready -ne 0 ]; then
		echo "Waiting for initialisation scripts to finish ($is_ready)"
		sleep 1
	    fi
	else
	    echo "Postgres startup appears to have failed"
	    exit 1
	fi
    done
}

# Run the script for postgres container
POSTGRES_PASSWORD=${POSTGRES_PASSWORD:-"secret"} /usr/local/bin/docker-entrypoint.sh $@&
pg_pid=$!

wait_until_db_ready
import_database

sudo -u _renderd /usr/bin/renderd -f &
renderd_pid=$!

/usr/sbin/apachectl start

stop_handler() {
    echo "Stopping services"
    kill -hup $renderd_pid
    /usr/sbin/apachectl stop
    sudo -u postgres /usr/lib/postgresql/15/bin/pg_ctl stop -D "$PGDATA"
    # ps aux
    # sleep 10
}

trap stop_handler HUP INT QUIT TERM

wait "$renderd_pid"
