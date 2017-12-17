/** 
 *  @file    OraclizeSoliditySqrtVerify.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "sqrt" verify benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking the "sqrt" Oraclize query type with verification.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract OraclizeSoliditySqrtVerify is OraclizeSolidity {
    
  //event LogEvent(string msg, uint[] res);

  function OraclizeSoliditySqrtVerify() public payable {}

  function sqrt(uint n) public {
    oracleQuery("sqrt", n, callback, true);	
  }

  function callback(string str) private {
    uint sqrt = parseInt(str);
    //LogEvent("oraclize", sqrt);
  }
}