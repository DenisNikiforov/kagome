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

KAGOME_HOME="${SCRIPT_DIR}/../.."

ACCOUNT_NAME="$1"
DEFAULT_CHAINSPEC=${KAGOME_HOME}/examples/network_x4/polkadot_2_local_nodes.raw.json
CHAINSPEC_FILE="$(realpath ${2:-${DEFAULT_CHAINSPEC}})"

docker run --rm \
	-v "${CHAINSPEC_FILE}":/chain-spec.json \
	-v "${SCRIPT_DIR}/${ACCOUNT_NAME}-node-key":/var/node-key \
	-p 9933:9933 \
	-p 9944:9944 \
	-p 30333:30333 \
	parity/polkadot  \
		--chain=/chain-spec.json \
		--${ACCOUNT_NAME} \
		-ldebug \
		-lwasm_overrides=info \
		-lwasmtime_cranelift::compiler=info \
		--node-key-file=/var/node-key
