# 
#  @file    oraclize_contract.sh
#  @author  Nicolas Lykke Iversen (nlykkei)
#  @date    1/1/2018  
#  @version 1.0 
#  
#  @brief Oraclize, Deploy script generator. 
#
#  @section DESCRIPTION
#  
#  Deploy script generator for Ethereum contracts using 'geth'.
#   - The generated script is loaded from 'geth' using loadScript() 
#   - The deployed contract is available as 'ci' in 'geth' console
#

#!/bin/bash

usage() {
    echo "Deploy script generator for Ethereum contracts using \'geth\'"
    echo "Usage: ./deploy_contract <contract> [option]..."
    echo "  -s, --solc=SOLC_PATH"
    echo "  -o, --output=OUTPUT_DIR"
    echo "  -p, --password=PASSWORD"
    echo "  -h, --help"
}

CONTRACT=""
SOLC="../../../build/solc/solc"
OUTPUT_DIR=~
PASSWORD=""
GAS_LIMIT="200000"
UNKNOWN_ARGS=() 

while [ $# -gt 0 ]
do
key="$1"
case $key in 
    -h|--help)
    usage
    exit 0
    ;;
    -s|--solc)
    SOLC="$2"
    shift # argument
    shift # value
    ;;
    -o|--output)
    OUTPUT_DIR="$2"
    shift # argument
    shift # value
    ;;
    -p|--password)
    PASSWORD="$2"
    shift # argument
    shift # value
    ;;
    -g|--gaslimit)
    GAS_LIMIT="$2"
    shift # argument
    shift # value
    ;;
    *)
    if [ -z ${CONTRACT} ]; then # empty-string
        CONTRACT="$1" # first positional argument
    else 
        UNKNOWN_ARGS+=("$1") # unknown argument
    fi
    shift # argument
    ;;
esac    
done

if [ -z ${CONTRACT} ]; then
    echo "Missing parameter: <contract>"
    usage
    exit 1
fi

set -- "${UNKNOWN_ARGS[@]}" # restore unknown arguments


#echo "CONTRACT=${CONTRACT} SOLC=${SOLC}"

CONTRACT_NAME=${CONTRACT##*/} 
CONTRACT_NAME=${CONTRACT_NAME%.*} # path/to/contract.sol => contract
SCRIPT_NAME="${OUTPUT_DIR}/${CONTRACT_NAME}.js"

#echo ${SCRIPT_NAME}

# redirect output to script
exec > ${SCRIPT_NAME}

# write contract ABI and BIN to .js file in JSON format
echo var contract_content =
./${SOLC} --oraclize --optimize --gaslimit ${GAS_LIMIT} --combined-json abi,bin,bin-runtime ${CONTRACT}

echo

# set default account (optional)
echo "eth.defaultAccount = eth.accounts[0]"

echo

# unlock account indefinitely (optional)
echo "personal.unlockAccount(eth.defaultAccount, \"${PASSWORD}\", 0)"

echo

# gas estimation
echo "var gas_value = eth.estimateGas({data:\"0x\"+contract_content.contracts[\"${CONTRACT}:${CONTRACT_NAME}\"].bin})"

echo

# contract template
echo "var ct = eth.contract(JSON.parse(contract_content.contracts[\"${CONTRACT}:${CONTRACT_NAME}\"].abi))"

echo

#contract instance
echo "
var ci = ct.new({from: eth.accounts[0], data: \"0x\" + contract_content.contracts[\"${CONTRACT}:${CONTRACT_NAME}\"].bin, gas: gas_value, value: 100000000000000000}, 
    function (e, contract) {
        console.log(e, contract);
        if (typeof contract.address !== 'undefined') {
            console.log('[Success]: Contract mined!\n[Address]: ' + contract.address + '\n[TxHash]: ' + contract.transactionHash);
        }
    }
);"

# functions in the contract are called from the 'geth' console follows:
# > ci.getFoo()
# > ci.updateFoo()











