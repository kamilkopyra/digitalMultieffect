#!/usr/bin/env bash
# Flashuje podany plik .bin na Daisy Seed przez DFU.
# Uzycie: ./flash.sh sciezka/do/pliku.bin
#
# Plytka musi byc w trybie DFU: przytrzymaj BOOT -> stuknij RESET -> pusc BOOT.

set -e

if [ -z "$1" ]; then
    echo "Uzycie: $0 sciezka/do/pliku.bin"
    exit 1
fi

BIN_FILE="$1"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DFU_UTIL="$SCRIPT_DIR/dfu-util-0.9-win64/dfu-util.exe"

if [ ! -f "$BIN_FILE" ]; then
    echo "Nie znaleziono pliku: $BIN_FILE"
    exit 1
fi

"$DFU_UTIL" -a 0 -s 0x08000000:leave -D "$BIN_FILE" -d 0483:df11
