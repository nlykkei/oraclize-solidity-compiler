/** 
 *  @file    OraclizeSolidityKDSUInt8.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "kds" benchmark (uint16). 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking the "kds" Oraclize query type (uint16).
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract OraclizeSolidityKDSUInt8 is OraclizeSolidity {
  
  //event LogEvent(string msg, uint[] res);

  function OraclizeSolidityKDSUInt8() public payable {}

  function kds(uint8[] m, uint k) public {
    oracleQuery("kds", m, k, callback);	
  }

  function callback(string str) private {
    uint[] memory kds = stringToArray(str);
    //LogEvent("oraclize", kds);
  }
}