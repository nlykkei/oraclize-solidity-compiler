/** 
 *  @file    OraclizeSolidityKPVerify.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "kp" verify benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking the "kp" Oraclize query type with verification.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract OraclizeSolidityKPVerify is OraclizeSolidity {
  
  //event LogEvent(string msg, uint[] res);

  function OraclizeSolidityKPVerify() public payable {}

  function kp(uint[] w, uint k, uint W) public {
    oracleQuery("kp", w, k, W, callback, true);	
  }

  function callback(string str) private {
    uint[] memory kp = stringToArray(str);
    //LogEvent("oraclize", kp);
  }
}