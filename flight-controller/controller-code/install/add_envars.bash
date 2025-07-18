# Export udp_capture ports to environment variables
# for HaFX science, debug,
# and X-123 science, debug

# simplifies coordination between udp_capture and
# the controller program

if [[ $# != 1 ]]; then
    echo "Need to supply folder with envars txt file in it as sole argument to $0."
    exit 1
fi

out_path="$HOME/detector-config"
mkdir -p "$HOME/detector-config"

base_path=$1
cp "$base_path/envars.bash" $out_path || (echo "Failed to copy file envars file" && exit -1)

source_str="source $out_path/envars.bash"
brc_path="$HOME/.bashrc"

if [[ !( $(cat $brc_path) =~ $source_str ) ]]; then
    echo $source_str >> $brc_path
    echo "updated envars file"
else
    echo "no change to envars"
fi


source $HOME/.bashrc
