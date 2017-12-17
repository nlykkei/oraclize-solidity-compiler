/** 
 *  @file    TestSqrt.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "sqrt" test. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for testing the "sqrt" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract TestSqrt is OraclizeSolidity {

  uint foo;
  uint bar;
  uint baz;

  uint n0 = 82;

  function TestSqrt() public payable {}

  function updateFoo() public {
    2 + 3;
    uint n1 = 17;
    oracleQuery("sqrt", n1, processFoo);	
    3 + 1;
  }

  function updateBar() public {
    oracleQuery("sqrt", n0, processBar, false, "http://oraclize-solidity.herokuapp.com/sqrt/");	
  }

  function updateBaz() public {
    oracleQuery("sqrt", 122, processBaz, true);	
  }

  function processFoo(string _str1) private {
    foo = parseInt(_str1);
  }

  function processBar(string _str1) private {
    bar = parseInt(_str1);
  }

  function processBaz(string _str1) private {
    baz = parseInt(_str1);
  }

  function getFoo() public constant returns(uint) {
    return foo;
  }

  function getBar() public constant returns(uint) {
    return bar;
  }

  function getBaz() public constant returns(uint) {
    return baz;
  }
}