/** 
 *  @file    OraclizeSoliditySort.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "sort" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking the "sort" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract OraclizeSoliditySort is OraclizeSolidity {
  
  //event LogEvent(string msg, uint[] res);

  function OraclizeSoliditySort() public payable {}

  function sort(uint[] nums) public {
    oracleQuery("sort", nums, callback);	
  }

  function callback(string str) private {
    uint[] memory nums = stringToArray(str);
    //LogEvent("oraclize", nums);
  }
}