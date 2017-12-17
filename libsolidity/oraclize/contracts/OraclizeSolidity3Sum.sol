/** 
 *  @file    OraclizeSolidity3Sum.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "3sum" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking the "3sum" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract OraclizeSolidity3Sum is OraclizeSolidity {
  
  //event LogEvent(string msg, uint[] res);

  function OraclizeSolidity3Sum() public payable {}

  function threeSum(uint[] nums, uint target) public {
    oracleQuery("3sum", nums, target, callback);	
  }

  function callback(string str) private {
    uint[] memory threeSum = stringToArray(str);
    //LogEvent("oraclize", threeSum);
  }
}