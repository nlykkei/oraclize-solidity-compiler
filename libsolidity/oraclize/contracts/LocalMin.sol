/** 
 *  @file    LocalMin.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Local "min" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking local "min".
 */

pragma solidity ^0.4.11;

contract LocalMin {
            
    //event LogEvent(string msg, uint res);

    function LocalMin() public payable {}    
    
    /// Local min
    function min(uint[] nums) public {
        uint _min = minimum(nums);
        //LogEvent("local", _min);
    }

    function minimum(uint[] arr) private constant returns(uint) {
        if (arr.length == 0) return 0;
        uint _min = arr[0];
        for (uint i = 1; i < arr.length; ++i) {
            if (arr[i] < _min) _min = arr[i]; 
        }
        return _min;
    }     
}