#!/bin/bash

read -r -d '' SH_COMMAND <<EOF
valkey-server \
    --unixsocket /var/run/valkey/valkey.sock \
    --unixsocketperm 777 \
    --port 0 \
    --daemonize yes && \
valkey-cli -s /var/run/valkey/valkey.sock
EOF

RUN_COMMAND=(
    docker run
    -it
    --rm
    -v /var/run/valkey:/var/run/valkey
    valkey/valkey:latest
    sh -c
)

"${RUN_COMMAND[@]}" "$SH_COMMAND"

