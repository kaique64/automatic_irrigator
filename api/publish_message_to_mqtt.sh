#!/bin/bash

NUM_MESSAGES=${1:-10}
MAX_THREADS=5

send_message() {
  local i=$1

  AIR_TEMP=$(awk -v min=15 -v max=40 'BEGIN{srand(); printf "%.1f", min + rand() * (max - min)}')
  AIR_HUM=$(awk -v min=30 -v max=95 'BEGIN{srand(); printf "%.1f", min + rand() * (max - min)}')
  SOIL_HUM=$(awk -v min=10 -v max=90 'BEGIN{srand(); printf "%.1f", min + rand() * (max - min)}')
  TIMESTAMP=$(date +%s%3N)

  PAYLOAD="{\"deviceId\": \"321\", \"timestamp\": $TIMESTAMP, \"air\": {\"temperature\": $AIR_TEMP, \"humidity\": $AIR_HUM}, \"soil\": {\"humidity\": $SOIL_HUM}}"

  echo "[$i/$NUM_MESSAGES] Enviando: $PAYLOAD"

  docker run --rm \
    --network api_irrigator_network hivemq/mqtt-cli pub \
    -h mqtt \
    -p 1883 \
    -t "greenhouse/sensors" \
    -m "$PAYLOAD"
}

running=0

for i in $(seq 1 $NUM_MESSAGES); do
  send_message $i &

  ((running++))

  if (( running >= MAX_THREADS )); then
    wait -n   # espera qualquer thread terminar
    ((running--))
  fi

  sleep 0.2
done

wait

echo "✅ $NUM_MESSAGES mensagens enviadas."