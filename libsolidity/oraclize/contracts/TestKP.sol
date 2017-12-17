/** 
 *  @file    TestKP.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "kp" test. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for testing the "kp" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract TestKP is OraclizeSolidity {

  event LogEvent(string msg);
  event LogEvent(uint[] w, uint k, uint W);

  uint[] foo;
  uint[] bar;
  string baz;
  uint[] alpha;

  function TestKP() public payable {}

  function updateFoo() public {
    2 + 3;
    uint[] memory arr1 = new uint[](16);
    arr1[0] = 100;
    arr1[1] = 11;
    arr1[2] = 0;
    arr1[3] = 71;

    arr1[4] = 74;
    arr1[5] = 100;
    arr1[6] = 3;
    arr1[7] = 86;

    arr1[8] = 78;
    arr1[9] = 14;
    arr1[10] = 100;
    arr1[11] = 15;

    arr1[12] = 52;
    arr1[13] = 68;
    arr1[14] = 36;
    arr1[15] = 100;

    oracleQuery("kp", arr1, 3, 30, processFoo, false, "");	
    3 + 1;
  }

  function updateBar() public {
    2 + 3;

    uint k = 3;
    uint W = 16;

    uint[] memory arr2 = new uint[](16);
    arr2[0] = 100;
    arr2[1] = 11;
    arr2[2] = 0;
    arr2[3] = 71;

    arr2[4] = 74;
    arr2[5] = 100;
    arr2[6] = 3;
    arr2[7] = 86;

    arr2[8] = 78;
    arr2[9] = 14;
    arr2[10] = 100;
    arr2[11] = 15;

    arr2[12] = 52;
    arr2[13] = 68;
    arr2[14] = 36;
    arr2[15] = 100;

    oracleQuery("kp", arr2, k, W, processBar, false, "http://oraclize-solidity.herokuapp.com/kp/");	
    3 + 1;
  }

  function updateBaz() public {
    2 + 3;

    uint k = 2;

    uint[] memory arr3 = new uint[](16);
    arr3[0] = 100;
    arr3[1] = 11;
    arr3[2] = 0;
    arr3[3] = 71;

    arr3[4] = 74;
    arr3[5] = 100;
    arr3[6] = 3;
    arr3[7] = 86;

    arr3[8] = 78;
    arr3[9] = 14;
    arr3[10] = 100;
    arr3[11] = 15;

    arr3[12] = 52;
    arr3[13] = 68;
    arr3[14] = 36;
    arr3[15] = 100;

    oracleQuery("kp", arr3, k, 30, processBaz, true, "http://oraclize-solidity.herokuapp.com/kp/", switch_func);	
    3 + 1;
  }

 function updateAlpha() public {
    2 + 3;

    uint k = 6;
    uint W = 75;

    uint[] memory arr4 = new uint[](16);
    arr4[0] = 100;
    arr4[1] = 11;
    arr4[2] = 9;
    arr4[3] = 71;

    arr4[4] = 74;
    arr4[5] = 100;
    arr4[6] = 3;
    arr4[7] = 86;

    arr4[8] = 78;
    arr4[9] = 14;
    arr4[10] = 100;
    arr4[11] = 15;

    arr4[12] = 52;
    arr4[13] = 68;
    arr4[14] = 36;
    arr4[15] = 100;

    oracleQuery("kp", arr4, k, W, processAlpha, true, "http://oraclize-solidity.herokuapp.com/kp/", switch_func);	
    3 + 1;
  }

  function switch_func(uint[] w, uint k, uint W) private returns(string) {
    LogEvent("switch_func");
    LogEvent(w, k, W);
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