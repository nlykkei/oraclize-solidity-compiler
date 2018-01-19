/** 
 *  @file    TestData.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "data" test. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for testing the "data" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract TestData is OraclizeSolidity {

  uint private foo;
  uint private bar;
  string private baz;

  function TestData() public payable {}

  /// Modified to query each URL and call specified callback with obtained results
  function updateFoo() public {
    2 + 3;
    oracleQuery("data", "http://oraclize-solidity.herokuapp.com/echo/2", "http://oraclize-solidity.herokuapp.com/echo/3", processFoo);
  }

  /// Modified to query each URL and call specified callback with obtained results
  function updateBar() public {
    oracleQuery("data", "http://oraclize-solidity.herokuapp.com/echo/4", "http://oraclize-solidity.herokuapp.com/echo/7", "http://oraclize-solidity.herokuapp.com/echo/1", processBar);	
  }

  /// Modified to query each URL and call specified callback with obtained results
  function updateBaz() public {
    oracleQuery("data", "http://oraclize-solidity.herokuapp.com/echo/HelloWorld", processBaz);	
  }

  /// User-supplied callback function
  function processFoo(string _str1, string _str2) private {
    foo = parseInt(_str1) * parseInt(_str2);
  }

  /// User-supplied callback function
  function processBar(string _str1, string _str2, string _str3) private {
    bar = parseInt(_str1) + parseInt(_str2) + parseInt(_str3);
  }

  /// User-supplied callback function
  function processBaz(string _str1) private {
    baz = _str1;
  }

  function getFoo() public constant returns(uint) {
    return foo;
  }

  function getBar() public constant returns(uint) {
    return bar;
  }

  function getBaz() public constant returns(string) {
    return baz;
  }
}