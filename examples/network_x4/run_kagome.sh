#!/usr/bin/env bash
set -euf -o pipefail

function realpath {
  if [[ -d "$1" ]]; then
    cd "$1" && pwd
  else
    echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
  fi
}

SCRIPT_PATH=$(realpath "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

KAGOME_HOME="$(dirname $(dirname ${SCRIPT_DIR}))"
KAGOME_BIN="${KAGOME_HOME}/build/node/kagome"

ACCOUNT_NAME="$1"
NETWORK_NAME="$2"
DEFAULT_CHAINSPEC="${KAGOME_HOME}/examples/network_x4/polkadot_2_local_nodes.raw.json"
CHAINSPEC="${CHAINSPEC:-${DEFAULT_CHAINSPEC}}"
LISTEN_PORT="${LISTEN_PORT:-11222}"
BOOTNODES="${BOOTNODES:-"/ip4/127.0.0.1/tcp/30333/p2p/12D3KooWC8t4TM6Ldk2jsqqRwUhSzNzD4p77s8a3wmEcGAepXrKs"}"

ARGS=("$@")
VAR_ARGS=("${ARGS[@]:2}")

rm -rf "${KAGOME_HOME}/examples/network_x4/${ACCOUNT_NAME}/${NETWORK_NAME}/db"

"${KAGOME_BIN}" \
	--chain "$CHAINSPEC" \
	-d "$KAGOME_HOME/examples/network_x4/$ACCOUNT_NAME"  \
	-ldebug --validator \
	--port ${LISTEN_PORT} \
	--rpc-port 11233 \
	--ws-port 11244 \
	--prometheus-port 11255 \
	-lgrandpa=debug \
	--bootnodes ${BOOTNODES}
