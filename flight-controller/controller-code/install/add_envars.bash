# Export udp_capture ports to environment variables
# for HaFX science, debug,
# and X-123 science, debug

# simplifies coordination between udp_capture and
# the controller program
cuser=${SUDO_USER:-$(whoami)}
out_path="/home/${cuser}/detector-config/umn_detector_envars.txt"
mkdir -p "$(dirname "$out_path")"
cp "/home/${cuser}/umn-detector-code/flight-controller/controller-code/install/envars.bash" $out_path
source_str="source $out_path"

if [[ !( $(cat "/home/${cuser}/.bashrc") =~ $source_str ) ]]; then
    echo $source_str >> "/home/${cuser}/.bashrc"
else
    echo "updated port"
fi

source /home/${cuser}/.bashrc
