#!/bin/bash

set -e

psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname "$POSTGRES_DB" <<EOF
-- CREATE USER trip WITH SUPERUSER CREATEDB CREATEROLE PASSWORD 'secret';
-- CREATE DATABASE trip OWNER=trip;
CREATE EXTENSION postgis;
EOF
sed -i -e "s/^\(\(local\|host\).*\(all\|replication\).*all.*trust\)/#\1/" /var/lib/postgresql/data/pg_hba.conf
