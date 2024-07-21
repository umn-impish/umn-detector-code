# Export udp_capture ports to environment variables
# for HaFX science, debug,
# and X-123 science, debug

# simplifies coordination between udp_capture and
# the controller program

if [[ $# != 1 ]]; then
    echo "Need to supply folder with envars txt file in it as sole argument to $0."
    exit 1
fi

cuser=${SUDO_USER:-$(whoami)}
out_path="/home/${cuser}/detector-config/umn_detector_envars.txt"
mkdir -p "$(dirname "$out_path")"


base_path=$1
cp "$base_path/envars.bash" $out_path || (echo "Failed to copy file envars file" && exit -1)


source_str="source $out_path"
brc_path="/home/${cuser}/.bashrc"

if [[ !( $(cat $brc_path) =~ $source_str ) ]]; then
    echo $source_str >> $brc_path
else
    echo "updated port"
fi


source /home/${cuser}/.bashrc
