/** 
 *  @file    TestMin.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "min" test. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for testing the "min" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract TestMin is OraclizeSolidity {

  uint foo;
  uint bar;
  uint baz;

  uint[] arr0;

  function TestMin() public payable {}

  function updateFoo() public {
    2 + 3;
    uint[] memory arr1 = new uint[](3);
    arr1[0] = 2;
    arr1[1] = 1;
    arr1[2] = 4;
    oracleQuery("min", arr1, processFoo);	
    3 + 1;
  }

  function updateBar() public {
    arr0.push(32);
    arr0.push(16);
    2 * 8;
    arr0.push(64);
    arr0.push(8);
    arr0.push(4);
    oracleQuery("min", arr0, processBar, "http://oraclize-solidity.herokuapp.com/min/");	
    delete arr0;
  }

  function updateBaz() public {
    2 + 3;
    uint[] memory arr2 = new uint[](5);
    arr2[0] = 2;
    2 + 6;
    arr2[1] = 2;
    1 < 3;
    arr2[2] = 7;
    arr2[3] = 5;
    arr2[4] = 3;
    oracleQuery("min", arr2, processBaz);		
  }

  function processFoo(string _str1) private {
    foo = unpack16Bit(_str1);
  }

  function processBar(string _str1) private {
    bar = unpack16Bit(_str1);
  }

  function processBaz(string _str1) private {
    baz = unpack16Bit(_str1);
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