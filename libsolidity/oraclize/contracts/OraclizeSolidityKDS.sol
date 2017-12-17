/** 
 *  @file    OraclizeSolidityKDS.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "kds" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking the "kds" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract OraclizeSolidityKDS is OraclizeSolidity {
  
  //event LogEvent(string msg, uint[] res);

  function OraclizeSolidityKDS() public payable {}

  function kds(uint[] m, uint k) public {
    oracleQuery("kds", m, k, callback);	
  }

  function callback(string str) private {
    uint[] memory kds = stringToArray(str);
    //LogEvent("oraclize", kds);
  }
}