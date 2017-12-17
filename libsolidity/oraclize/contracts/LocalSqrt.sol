/** 
 *  @file    LocalSqrt.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Local "sqrt" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking local "sqrt".
 */

pragma solidity ^0.4.11;

contract LocalSqrt {
    
    //event LogEvent(string msg, uint res);

    function LocalSqrt() public payable {}    
   
    /// Local sqrt
    function sqrt(uint n) public {
        uint _sqrt = babylonian(n);
        //LogEvent("local", sqrt);
    }

    /// Babylonian method
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