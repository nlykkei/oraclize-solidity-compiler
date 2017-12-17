/** 
 *  @file    LocalAPSP.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Local "apsp" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking local "apsp".
 */

pragma solidity ^0.4.11;

contract LocalAPSP {

    //event LogEvent(string msg, int[] res);

    int constant INF = -1;

    function LocalAPSP() public payable {}    

    /// Local all-pairs shortest path
    function apsp(int[] w) public {
        int[][] memory _apsp  = fw(w);
        //for (uint i = 0; i < _apsp.length; ++i) //LogEvent("local", _apsp[i]); // Infinity encoded as -1
    }
  
    /// Floyd-Warshall
    function fw(int[] w) private constant returns(int[][]) {
        uint i; uint j; uint k;
        uint n = babylonian(w.length);
        int[][] memory d = new int[][](n);
        
        for (i = 0; i < n; ++i) {
            d[i] = new int[](n);
            for (j = 0; j < n; ++j) {
                d[i][j] = (w[i * n + j] >= 100 ? INF : w[i * n + j]);
            }   
            d[i][i] = 0;
        }
        
        for (k = 0; k < n; ++k) {
            for (i = 0; i < n; ++i) {
                for (j = 0; j < n; ++j) {
                    if (d[i][k] == INF || d[k][j] == INF) continue;
                    else if (d[i][j] == INF) d[i][j] = d[i][k] + d[k][j];
                    else d[i][j] = min(d[i][j], d[i][k] + d[k][j]);
                }
            }
        }
        return d;
    }
    
    function min(int a, int b) private constant returns(int)  {
        return a < b ? a : b;
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