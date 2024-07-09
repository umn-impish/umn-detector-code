# Export udp_capture ports to environment variables
# for HaFX science, debug,
# and X-123 science, debug

# simplifies coordination between udp_capture and
# the controller program

cp "detector_ports.bash" "$HOME/detector-config/umn_detector_ports.txt"
source_str='source detector-config/umn_detector_ports.txt'

if [[ !( $(cat "$HOME/.bashrc") =~ $source_str ) ]]; then
    echo $source_str >> "$HOME/.bashrc"
else
    echo "ports already installed"
fi
