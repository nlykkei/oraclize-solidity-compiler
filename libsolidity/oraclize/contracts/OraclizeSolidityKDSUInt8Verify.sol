/** 
 *  @file    OraclizeSolidityKDSUInt8Verify.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "kds" verify benchmark (uint16). 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking the "kds" Oraclize query type with verification (uint16).
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract OraclizeSolidityKDSUInt8Verify is OraclizeSolidity {
  
  //event LogEvent(string msg, uint[] res);

  function OraclizeSolidityKDSUInt8Verify() public payable {}

  function kds(uint8[] m, uint k) public {
    oracleQuery("kds", m, k, callback, true);	
  }

  function callback(string str) private {
    uint[] memory kds = stringToArray(str);
    //LogEvent("oraclize", kds);
  }
}