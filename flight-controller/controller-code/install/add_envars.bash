# Export udp_capture ports to environment variables
# for HaFX science, debug,
# and X-123 science, debug

# simplifies coordination between udp_capture and
# the controller program

out_path="$HOME/detector-config/umn_detector_envars.txt"
cp "envars.bash" $out_path
source_str="source $out_path"

if [[ !( $(cat "$HOME/.bashrc") =~ $source_str ) ]]; then
    echo $source_str >> "$HOME/.bashrc"
else
    echo "ports already installed"
fi
