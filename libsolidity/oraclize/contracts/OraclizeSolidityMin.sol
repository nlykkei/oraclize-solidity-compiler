/** 
 *  @file    OraclizeSolidityMin.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "min" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking the "min" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract OraclizeSolidityMin is OraclizeSolidity {
  
  //event LogEvent(string msg, uint res);

  function OraclizeSolidityMin() public payable {}

  function min(uint[] nums) public {
    oracleQuery("min", nums, callback);	
  }

  function callback(string str) private {
    uint min = unpack16Bit(str);
    //LogEvent("oraclize", min);
  }
}