/** 
 *  @file    TestSort.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "sort" test. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for testing the "sort" Oraclize query type.
 */


pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract TestSort is OraclizeSolidity {

  uint[] foo;
  uint[] bar;

  uint[] arr0;

  function TestSort() public payable {}

  function updateFoo() public {
    2 + 3;
    uint[] memory arr1 = new uint[](3);
    arr1[0] = 2;
    arr1[1] = 1;
    arr1[2] = 4;
    oracleQuery("sort", arr1, processFoo);	
    3 + 1;
  }

  function updateBar() public {
    arr0.push(32);
    arr0.push(16);
    2 * 8;
    arr0.push(64);
    arr0.push(8);
    arr0.push(4);
    oracleQuery("sort", arr0, processBar, false, "http://oraclize-solidity.herokuapp.com/sort/");	
    delete arr0;
  }

  function processFoo(string _str1) private {
    foo = stringToArray(_str1);
  }

  function processBar(string _str1) private {
    bar = stringToArray(_str1);
  }

  function getFoo() public constant returns(uint[]) {
    return foo;
  }

  function getBar() public constant returns(uint[]) {
    return bar;
  }
}