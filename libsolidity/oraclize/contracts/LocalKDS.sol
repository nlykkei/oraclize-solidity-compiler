/** 
 *  @file    LocalKDS.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Local "kds" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking local "kds".
 */

pragma solidity ^0.4.11;

contract LocalKDS {
            
    //event LogEvent(string msg, uint[] res);

    function LocalKDS() public payable {}    
  
    /// Local k-path
    function kds(uint[] m, uint k) public {
        uint[] memory _kds  = kdset(m, k);
        //LogEvent("local", _kds);
    }
    
    /// Dominating set DS of size <= k 
    function kdset(uint[] m, uint k) private constant returns(uint[]) {
        uint i; uint j; uint sz; 
        uint n = babylonian(m.length);
        uint total = 2**n;
        uint[] memory dset = new uint[](n);
        uint[] memory set = new uint[](n); 
        bool[] memory dominated = new bool[](n);
        
        // Generate all vertex combinations
        for (uint mask = 1; mask < total; ++mask) { // O(2^n)
            j = 0; i = n - 1;
            do {
                if ((mask & (1 << i)) != 0) { // 001, 010, 011, 100, 101, 110, 111
                    set[j++] = i;
                }
            } while (i-- > 0);
            
            if (j > k) continue; // Set too large    
            else {
                for (i = 0; i < n; ++i) dominated[i] = false; // Reset domination
                for (uint v = 0; v < j; ++v) {
                    dominated[set[v]] = true; 
                    for (uint u = 0; u < n; u++) {
                        if (m[set[v] * n + u] != 0) {
                            dominated[u] = true; // Dominated vertex
                        }
                    }
                }
            }
            
            for (i = 0; i < n; ++i) {
                if (dominated[i] == false) {
                    break;
                }
            }

            if (i >= n) { // Dominating set
                if (sz == 0 || j < sz) {
                    for (i = 0; i < j; ++i) dset[i] = set[i];
                    sz = j;
                }
            } 
        }
        
        uint[] memory res = new uint[](sz);
        for (i = 0; i < sz; ++i) res[i] = dset[i];
        return res;
    }

    function babylonian(uint n) private constant returns(uint) {
        uint x = n;
        uint y = 1;
        while (x > y) {
            x = (x + y) / 2;
            y = n / x;
        }
        return x;
    }
}