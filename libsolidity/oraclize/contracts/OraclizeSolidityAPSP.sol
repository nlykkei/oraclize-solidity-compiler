/** 
 *  @file    OraclizeSolidityAPSP.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "apsp" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking the "apsp" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract OraclizeSolidityAPSP is OraclizeSolidity {

  //event LogEvent(string msg, uint[] res);

  function OraclizeSolidityAPSP() public payable {}

  function apsp(uint[] w) public {
    oracleQuery("apsp", w, callback);	
  }

  function callback(string str) private {
    uint[] memory apsp = stringToArray(str);
    //LogEvent("oraclize", apsp);
  }
}