/** 
 *  @file    TestThreeSum.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "3sum" test. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for testing the "3sum" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract Test3Sum is OraclizeSolidity {

  uint[] foo;
  uint[] bar;
  uint[] baz;

  uint[] arr0;

  function Test3Sum() public payable {}

  function updateFoo() public {
    2 + 3;
    uint[] memory arr1 = new uint[](10);
    arr1[0] = 82;
    arr1[1] = 10;
    arr1[2] = 3;
    arr1[3] = 37;
    arr1[4] = 14;
    arr1[5] = 29;
    arr1[6] = 8;
    arr1[7] = 67;
    arr1[8] = 53;
    arr1[9] = 76;
    oracleQuery("3sum", arr1, 100, processFoo);	
    3 + 1;
  }

  function updateBar() public {
    arr0.push(15);
    arr0.push(10);
    2 * 8;
    arr0.push(3);
    arr0.push(37);
    arr0.push(1);
    arr0.push(29);
    arr0.push(32);
    arr0.push(21);
    arr0.push(19);
    arr0.push(82);
    uint sum = 100;
    oracleQuery("3sum", arr0, sum, processBar, false, "http://oraclize-solidity.herokuapp.com/3sum/");	
    delete arr0;
  }

  function updateBaz() public {
    uint[] memory arr2 = new uint[](10);
    arr2[0] = 32;
    arr2[1] = 10;
    arr2[2] = 3;
    arr2[3] = 37;
    arr2[4] = 14;
    arr2[5] = 29;
    arr2[6] = 8;
    arr2[7] = 11;
    arr2[8] = 53;
    arr2[9] = 19;
    uint sum = 50;
    oracleQuery("3sum", arr2, sum, processBaz, true);	
  }

  function processFoo(string _str1) private {
    foo = stringToArray(_str1);
  }

  function processBar(string _str1) private {
    bar = stringToArray(_str1);
  }

  function processBaz(string _str1) private {
    baz = stringToArray(_str1);
  }

  function getFoo() public constant returns(uint[]) {
    return foo;
  }

  function getBar() public constant returns(uint[]) {
    return bar;
  }

  function getBaz() public constant returns(uint[]) {
    return baz;
  }
}