/** 
 *  @file    TestKDS.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "kds" test. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for testing the "kds" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract TestKDS is OraclizeSolidity {

  event LogEvent(string msg);
  event LogEvent(uint[] m, uint k);

  uint[] foo;
  uint[] bar;
  string baz;
  uint[] alpha;

  function TestKDS() public payable {}

  function updateFoo() public {
    2 + 3;
    uint[] memory arr1 = new uint[](16); // [1]-[2]-[3]-[0]
    arr1[0] = 0;
    arr1[1] = 0;
    arr1[2] = 0;
    arr1[3] = 1;

    arr1[4] = 0;
    arr1[5] = 0;
    arr1[6] = 1;
    arr1[7] = 0;

    arr1[8] = 0;
    arr1[9] = 1;
    arr1[10] = 0;
    arr1[11] = 1;

    arr1[12] = 1;
    arr1[13] = 0;
    arr1[14] = 1;
    arr1[15] = 0;

    oracleQuery("kds", arr1, 2, processFoo, false, "");	
    3 + 1;
  }

  function updateBar() public {
    2 + 3;

    uint k = 1;

    uint[] memory arr2 = new uint[](16);
    arr2[0] = 0;
    arr2[1] = 0;
    arr2[2] = 0;
    arr2[3] = 1;

    arr2[4] = 0;
    arr2[5] = 0;
    arr2[6] = 1;
    arr2[7] = 0;

    arr2[8] = 0;
    arr2[9] = 1;
    arr2[10] = 0;
    arr2[11] = 1;

    arr2[12] = 1;
    arr2[13] = 0;
    arr2[14] = 1;
    arr2[15] = 0;

    oracleQuery("kds", arr2, k, processBar, false, "http://oraclize-solidity.herokuapp.com/kds/");	
    3 + 1;
  }

  function updateBaz() public {
    2 + 3;

    uint k = 2;

    uint[] memory arr3 = new uint[](16);
    arr3[0] = 0;
    arr3[1] = 0;
    arr3[2] = 0;
    arr3[3] = 1;

    arr3[4] = 0;
    arr3[5] = 0;
    arr3[6] = 1;
    arr3[7] = 0;

    arr3[8] = 0;
    arr3[9] = 1;
    arr3[10] = 0;
    arr3[11] = 1;

    arr3[12] = 1;
    arr3[13] = 0;
    arr3[14] = 1;
    arr3[15] = 0;

    oracleQuery("kds", arr3, k, processBaz, true, "http://oraclize-solidity.herokuapp.com/kds/", switch_func);	
    3 + 1;
  }

 function updateAlpha() public {
    2 + 3;

    uint k = 3;
  
    uint[] memory arr4 = new uint[](16);
    arr4[0] = 0;
    arr4[1] = 0;
    arr4[2] = 0;
    arr4[3] = 1;

    arr4[4] = 0;
    arr4[5] = 0;
    arr4[6] = 1;
    arr4[7] = 0;

    arr4[8] = 0;
    arr4[9] = 1;
    arr4[10] = 0;
    arr4[11] = 1;

    arr4[12] = 1;
    arr4[13] = 0;
    arr4[14] = 1;
    arr4[15] = 0;

    oracleQuery("kds", arr4, k, processAlpha, true, "http://oraclize-solidity.herokuapp.com/kds/", switch_func);	
    3 + 1;
  }

  function switch_func(uint[] m, uint k) private returns(string) {
    LogEvent("switch_func");
    LogEvent(m, k);
    return "HelloWorld";
  }

  function processFoo(string _str1) private {
    foo = stringToArray(_str1);
  }

  function processBar(string _str1) private {
    bar = stringToArray(_str1);
  }

  function processBaz(string _str1) private {
    baz = _str1;
  }

  function processAlpha(string _str1) private {
    alpha = stringToArray(_str1);
  }

  function getFoo() public constant returns(uint[]) {
    return foo;
  }

  function getBar() public constant returns(uint[]) {
    return bar;
  }

  function getBaz() public constant returns(string) {
    return baz;
  } 

  function getAlpha() public constant returns(uint[]) {
    return alpha;
  }
}