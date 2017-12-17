/** 
 *  @file    OraclizeSolidityKPUInt16.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "kp" benchmark (uint16). 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking the "kp" Oraclize query type (uint16).
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract OraclizeSolidityKPUInt16 is OraclizeSolidity {
  
  //event LogEvent(string msg, uint[] res);

  function OraclizeSolidityKPUInt16() public payable {}

  function kp(uint16[] w, uint k, uint W) public {
    oracleQuery("kp", w, k, W, callback);	
  }

  function callback(string str) private {
    uint[] memory kp = stringToArray(str);
    //LogEvent("oraclize", kp);
  }
}