/**
 * Retrieves a list of transactions for specified account.
 * Only transactions in blocks within range are retrieved. 
 *
 * @param {string} myaccount the account.
 * @param {int} startBlockNumber start of range 
 * @param {int} endBlockNumber end of range
 * @returns {void} 
 */

function getTransactionsByAccount(myaccount, startBlockNumber, endBlockNumber) {
    if (endBlockNumber == null) {
        endBlockNumber = eth.blockNumber;
        console.log("Using endBlockNumber: " + endBlockNumber);
    }
    if (startBlockNumber == null) {
        startBlockNumber = endBlockNumber - 100;
        if (startBlockNumber < 0) startBlockNumber = 0;
        console.log("Using startBlockNumber: " + startBlockNumber);
    }

    console.log("Searching for transactions to/from account \"" + myaccount + "\" within blocks " + startBlockNumber + " and " + endBlockNumber);

    for (var i = startBlockNumber; i <= endBlockNumber; i++) {
        if (i % 1000 == 0) {
            console.log("Searching block " + i);
        }
        var block = eth.getBlock(i, true);
        if (block != null && block.transactions != null) {
            block.transactions.forEach(function (e) {
                if (myaccount == "*" || myaccount == e.from || myaccount == e.to) {
                    var receipt = eth.getTransactionReceipt(e.hash);
                    console.log(""
                        + "   tx-hash          : " + receipt.transactionHash + "\n"
                        + "   from             : " + receipt.from + "\n"
                        + "   to               : " + receipt.to + "\n"
                        + "   gas              : " + e.gas + "\n"
                        + "   gas-used         : " + receipt.gasUsed + "\n"
                        + "   cumm-gas         : " + receipt.cumulativeGasUsed + "\n"
                        + "   gas-price        : " + e.gasPrice + "\n"
                        + "   contract         : " + receipt.contractAddress + "\n");
                }
            });
        }
    }
}

