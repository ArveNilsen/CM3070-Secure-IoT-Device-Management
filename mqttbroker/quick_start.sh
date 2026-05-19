# Check configuration in /etc/mosquitto/mosquitto.conf

# Start broker
mosquitto

# Test subscribing to topic
mosquitto_sub -t 'test/topic' -v

# Test publishing topic
mosquitto_pub -t 'test/topic' -m 'hello world'
