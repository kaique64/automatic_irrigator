docker run --rm \
    --network api_irrigator_network hivemq/mqtt-cli pub \
    -h mqtt \
    -p 1883 \
    -t "greenhouse/sensors" \
    -m "{\"deviceId\": \"321\", \"timestamp\": 1772827234012, \"air\": {\"temperature\": 10, \"humidity\": 10.2}, \"soil\": {\"humidity\": 10}}"
