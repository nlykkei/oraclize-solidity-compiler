/** 
 *  @file    TestAPSP.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "apsp" test. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for testing the "apsp" Oraclize query type.
 */

pragma solidity ^0.4.11;
import "./OraclizeSolidity.sol";

contract TestAPSP is OraclizeSolidity {
  
  uint[] foo;
  uint[] bar;

  function TestAPSP() public payable {}

  function updateFoo() public {
    2 + 3;
    uint[] memory arr0 = new uint[](16);
    arr0[0] = 100;
    arr0[1] = 11;
    arr0[2] = 0;
    arr0[3] = 71;

    arr0[4] = 74;
    arr0[5] = 100;
    arr0[6] = 3;
    arr0[7] = 86;

    arr0[8] = 78;
    arr0[9] = 14;
    arr0[10] = 100;
    arr0[11] = 15;

    arr0[12] = 52;
    arr0[13] = 68;
    arr0[14] = 36;
    arr0[15] = 100;

    oracleQuery("apsp", arr0, processFoo);	
    3 + 1;
  }

  function updateBar() public {
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

    oracleQuery("apsp", arr1, processBar, "http://oraclize-solidity.herokuapp.com/apsp/");	
    3 + 1;
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