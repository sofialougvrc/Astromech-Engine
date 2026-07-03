#!/usr/bin/env bash
set -euo pipefail

ACE_HEADER="include/ace/actuator_serial.hpp"
ARDUINO_SKETCH="firmware/arduino_ace_slave/arduino_ace_slave.ino"
CONFIG_SAMPLE="config/actuators.example.json"

ace_baud="$(sed -nE 's/.*int baud_rate = ([0-9]+);.*/\1/p' "$ACE_HEADER" | head -1)"
arduino_baud="$(sed -nE 's/.*BAUD_RATE = ([0-9]+);.*/\1/p' "$ARDUINO_SKETCH" | head -1)"

if [[ -z "$ace_baud" || -z "$arduino_baud" ]]; then
  echo "Unable to read baud constants" >&2
  exit 1
fi

if [[ "$ace_baud" != "$arduino_baud" ]]; then
  echo "Baud mismatch: ACE=$ace_baud Arduino=$arduino_baud" >&2
  exit 1
fi

if ! grep -q "\"baud_rate\": $ace_baud" "$CONFIG_SAMPLE"; then
  echo "Sample actuator config does not include baud_rate $ace_baud" >&2
  exit 1
fi

echo "baud-rate check: $ace_baud ok"
