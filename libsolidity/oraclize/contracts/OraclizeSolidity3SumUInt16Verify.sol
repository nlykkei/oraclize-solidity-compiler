/** 
 *  @file    OraclizeSolidity3SumUInt16Verify.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "3sum" verify benchmark (uint16). 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking the "3sum" Oraclize query type with verification (uint16).
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract OraclizeSolidity3SumUInt16Verify is OraclizeSolidity {
  
  //event LogEvent(string msg, uint[] res);

  function OraclizeSolidity3SumUInt16Verify() public payable {}

  function threeSum(uint16[] nums, uint32 sum) public {
    oracleQuery("3sum", nums, sum, callback, true);	
  }

  function callback(string str) private {
    uint[] memory threeSum = stringToArray(str);
    //LogEvent("oraclize", threeSum);
  }
}