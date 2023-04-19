#!/bin/bash
set -e

if [ -r "$POSTGRES_PASSWORD_FILE" ]; then
    POSTGRES_PASSWORD=$(cat ${POSTGRES_PASSWORD_FILE})
fi
if [ -r "$TRIP_SIGNING_KEY_FILE" ];then
    TRIP_SIGNING_KEY=$(cat ${TRIP_SIGNING_KEY_FILE})
fi
if [ -r "$TRIP_RESOURCE_SIGNING_KEY_FILE" ];then
    TRIP_RESOURCE_SIGNING_KEY=$(cat ${TRIP_RESOURCE_SIGNING_KEY_FILE})
fi

umask 0026
cat > trip-server.yaml <<EOF
app:
  json:
    indent:
      level: 0
  gpx:
    allowInvalidXsd: false
  autoQuit:
    timeOut: 0
  token:
    expiresIn: 43200
    renewWithin: 21600
  resourceToken:
    expiresIn: 86400
  maxItinerarySearchRadius: 50000
  averageFlatSpeedKph: 4
jwt:
EOF

echo "  signingKey: \"${TRIP_SIGNING_KEY}\"" >> trip-server.yaml
echo "  resourceSigningKey: \"${TRIP_RESOURCE_SIGNING_KEY}\"" >> trip-server.yaml

if [ "$CONFIGURE_TILE_SERVER" == "yes" ];
then
    cat >>trip-server.yaml <<EOF
tile:
  cache:
    maxAge: 0
  providers:
    - cache: false
      prune: false
      # see http://wiki.openstreetmap.org/wiki/Tile_usage_policy"
      userAgentInfo: (mailto:support@fdsd.co.uk)
      refererInfo: https://www.fdsd.co.uk/trip/about.html
      options:
        protocol: "http:"
        host: map
        port: "80"
        path: /tile/{z}/{x}/{y}.png
        method: GET
      mapLayer:
        name: Switch2OSM
        type: xyz
        minZoom: 0
        maxZoom: 19
        tileAttributions:
          - text: "Map data &copy; "
          - text: OpenStreetMap
            link: http://openstreetmap.org
          - text: " contributors, "
          - text: CC-BY-SA
            link: http://creativecommons.org/licenses/by-sa/2.0/
EOF
else
    cat >>trip-server.yaml <<EOF
tile:
  cache:
    maxAge: 0
  providers_READ_TILE_USAGE_POLICY_BELOW_AND_SET_USER_AGENT_AND_REFERRER_INFO_BEFORE_USING:
    - help: " see http://wiki.openstreetmap.org/wiki/Tile_usage_policy"
      userAgentInfo: (mailto:your.contact@email.address)
      refererInfo: http://link_to_deployed_application_information
      options:
        protocol: "http:"
        host: tile.openstreetmap.org
        port: "80"
        path: /{z}/{x}/{y}.png
        method: GET
      mapLayer:
        name: OSM Mapnik
        type: xyz
        tileAttributions:
          - text: "Map data &copy; "
          - text: OpenStreetMap
            link: http://openstreetmap.org
          - text: " contributors, "
          - text: CC-BY-SA
            link: http://creativecommons.org/licenses/by-sa/2.0/
EOF
fi

cat >>trip-server.yaml <<EOF
db:
EOF

echo "  uri: postgresql://trip:${POSTGRES_PASSWORD}@postgis/trip" >>trip-server.yaml

cat >>trip-server.yaml <<EOF
  poolSize: 10
  poolIdleTimeout: 10000
  connectionTimeoutMillis: 0
elevation-local-disabled:
  tileCacheMs: 60000
  datasetDir: ./elevation-data/
elevation-remote-disabled:
  provider:
    userAgentInfo: (mailto:your.contact@email.address)
    refererInfo: http://link_to_deployed_application_information
    options:
      protocol: "http:"
      host: localhost
      port: "8082"
      path: /api/v1/lookup
      method: POST
staticFiles:
  allow: true
debug: false
log:
  level: info
validation:
  headers:
    contentType:
      warn: true
reporting:
  metrics:
    tile:
      count:
        frequency: 100
EOF

# When running a development environment
if [ -f /webapp/package.json ]
then
    DIR=$(pwd)
    cd /webapp
    yarn
    cd /app-server
    if [ ! -L /webapp/app ]
    then
	if [ -e app ]
	then
	    rm -rf app
	fi
	ln -s /webapp/app
    fi
    cd "$DIR"
fi

trip-server --upgrade

exec "$@"
