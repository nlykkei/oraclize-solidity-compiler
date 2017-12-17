/** 
 *  @file    LocalKP.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Local "kp" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking local "kp".
 */

pragma solidity ^0.4.11;

contract LocalKP {
            
    //event LogEvent(string msg, uint[] res);

    int constant INF = -1;

    function LocalKP() public payable {}    
  
    /// Local k-path
    function kp(int[] w, uint k, uint W) public {
        uint[] memory _kp = kpath(w, k, W);
        //LogEvent("local", _kp);
    }
    
    /// Path P of length k, w(P) <= W 
    function kpath(int[] w, uint k, uint W) private constant returns(uint[]) {
        uint i; uint j; uint e; uint a;
        uint n = babylonian(w.length);
        int[][][] memory sp;
        
        sp = new int[][][](n);
        
        for (i = 0; i < n; ++i) {
            sp[i] = new int[][](n);
            for (j = 0; j < n; ++j) {
                sp[i][j] = new int[](k + 1);
            }
        }
        
        for (i = 0; i < n; ++i) {
            for (j = 0; j < n; ++j) {
                for (e = 0; e <= k; ++e) {
                    sp[i][j][e] = INF; // Initialize all entries to INF
                }
                if (w[i * n + j] >= 100) w[i * n + j] = INF;
                else sp[i][j][1] = w[i * n + j];
            }
            sp[i][i][0] = 0;
        }

        for (e = 2; e <= k; ++e) {
            for (i = 0; i < n; ++i) {
                for (j = 0; j < n; ++j) {
                    for (a = 0; a < n; ++a) {
                        if (sp[a][j][e - 1] == INF || w[i * n + a] == INF) continue;
                        else if (sp[i][j][e] == INF) sp[i][j][e] = w[i * n + a] + sp[a][j][e - 1];
                        else sp[i][j][e] = min(sp[i][j][e], w[i * n + a] + sp[a][j][e - 1]);
                    }
                }
            }
        }
        return getPath(w, k, W, sp, n);
    }
    
    /// Path reconstruction    
    function getPath(int[] w, uint k, uint W, int[][][] sp, uint n) private constant returns(uint[]) {
        uint i; uint j; uint e; uint a;
        uint[] memory path = new uint[](k + 1);
        uint src;
        uint dest;
        int sp_len = INF;

        for (i = 0; i < n; ++i) {
            for (j = 0; j < n; ++j) {
                if (sp_len == INF || sp_len > sp[i][j][k]) {
                    sp_len = sp[i][j][k];
                    src = i;
                    dest = j;
                }
            }
        }

        if (sp_len == INF || sp_len > int(W)) {
            return new uint[](0); // No path of lenght 'k' with weight less than 'W'
        }

        i = 0;
        path[i++] = src;
        
        for (e = k; e >= 2; --e) {
            for (a = 0; a < n; ++a) {
                if (w[src * n + a] == INF) continue;
                if (sp_len == (w[src * n + a] + sp[a][dest][e - 1])) {
                    path[i++] = a;
                    src = a;
                    sp_len = sp[a][dest][e - 1];
                }
            }
        }
        
        path[i] = dest;
        return path;        
    }

    function min(int a, int b) private constant returns(int) {
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