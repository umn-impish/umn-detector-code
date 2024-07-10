# port that detector controller listens on
# listens on ip INADDR_ANY
det_port=$DET_SERVICE_PORT

# Pipe commands to here
det_udp_dev="/dev/udp/127.0.0.1/$det_port"
