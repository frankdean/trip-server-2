#!/bin/bash

set -e

psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname "$POSTGRES_DB" <<EOF
CREATE EXTENSION postgis;
CREATE EXTENSION hstore;
EOF
touch "$PGDATA/db-setup-complete"
