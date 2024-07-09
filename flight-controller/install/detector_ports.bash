# Detector ports - to be sourced in .bashrc

# Ports in [base_port, base_port + 999] can be used
base_port=61000

export DET_SERVICE_PORT=$(($base_port + 999))


offest=0
export HAFX_C1_SCI_PORT=$(($base_port + $offset++))
export HAFX_C1_DBG_PORT=$(($base_port + $offset++))

export HAFX_M1_SCI_PORT=$(($base_port + $offset++))
export HAFX_M1_DBG_PORT=$(($base_port + $offset++))

export HAFX_M5_SCI_PORT=$(($base_port + $offset++))
export HAFX_M5_DBG_PORT=$(($base_port + $offset++))

export HAFX_X1_SCI_PORT=$(($base_port + $offset++))
export HAFX_X1_DBG_PORT=$(($base_port + $offset++))

export X123_SCI_PORT=$(($base_port + $offset++))
export X123_DBG_PORT=$(($base_port + $offset++))
