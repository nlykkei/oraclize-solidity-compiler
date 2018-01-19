/** 
 *  @file    TestDataMod.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, "data" contract modification. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for demonstrating the "data" Oraclize query type transformations.
 *  TestData.sol is the modified contract.
 */

pragma solidity ^0.4.11;
import "./oraclizeAPI.sol";

contract TestDataMod is usingOraclize 
{
    event OraclizeEvent(bytes32 queryId, string description);   

    struct OEnv0 
    {
        string[2] urls;
        string[2] retVals;
        uint8 index;
        bytes32 queryId;
    }
    
    struct OEnv1 
    {
        string[3] urls;
        string[3] retVals;
        uint8 index;
        bytes32 queryId;
    }  

    struct OEnv2 
    {
        bytes32 queryId;
    }      
    
    OEnv1 private _oEnv0;
    OEnv2 private _oEnv1;
    OEnv2 private _oEnv2;

    uint private foo;
    uint private bar;
    uint private baz;

    function TestDataMod() public payable {}

    /// Modified to query each URL and call specified callback with obtained results
    function updateFoo() public { 
      _oEnv0 = OEnv0({
          urls: ["http://oraclize-solidity.herokuapp.com/echo/2", "http://oraclize-solidity.herokuapp.com/echo/3"],
          retVals: ["", ""],
          index: 0,
          queryId: bytes32(0)});

      _oEnv0.queryId = oraclize_query("URL", _oEnv0.urls[_oEnv0.index]);

      OraclizeEvent(_oEnv0.queryId, "updateFoo");
      OraclizeEvent(_oEnv0.queryId, _oEnv0.urls[_oEnv0.index]);
    }

    /// Modified to query each URL and call specified callback with obtained results
    function updateBar() public {
      _oEnv1 = OEnv1({
          urls: ["http://oraclize-solidity.herokuapp.com/echo/4", "http://oraclize-solidity.herokuapp.com/echo/7", "http://oraclize-solidity.herokuapp.com/echo/1"],
          retVals: ["", "", ""],
          index: 0,
          queryId: bytes32(0)});

      _oEnv1.queryId = oraclize_query("URL", _oEnv1.urls[_oEnv1.index]);

      OraclizeEvent(_oEnv1.queryId, "updateBar");
      OraclizeEvent(_oEnv1.queryId, _oEnv1.urls[_oEnv1.index]);	
    }

    /// Modified to query each URL and call specified callback with obtained results
    function updateBaz() public {
      _oEnv2 = OEnv2({queryId: bytes32(0)});

      _oEnv2.queryId = oraclize_query("URL", "http://oraclize-solidity.herokuapp.com/echo/HelloWorld");

      OraclizeEvent(_oEnv2.queryId, "updateBaz");
      OraclizeEvent(_oEnv2.queryId, "http://oraclize-solidity.herokuapp.com/echo/HelloWorld");	
    }

    /// User-supplied callback function
    function processFoo(string _str1, string _str2) private {
      foo = parseInt(_str1) * parseInt(_str2);
    }

    // User-supplied callback function
    function processBar(string _str1, string _str2, string _str3) private {
      bar = parseInt(_str1) + parseInt(_str2) + parseInt(_str3);
    }

    // User-supplied callback function
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

    function _callback(bytes32 _queryId, string _result) public {
      OraclizeEvent(_queryId, "_callback() received:");
      OraclizeEvent(_queryId, _result);		
      
      if (_queryId == _oEnv0.queryId) {
        _oEnv0.retVals[_oEnv0.index] = _result;
        _oEnv0.index += 1;
        if (_oEnv0.index == 2) {
          processFoo(_oEnv0.retVals[0], _oEnv0.retVals[1]);
          delete _oEnv0; // Reclaim storage costs
        } else {
          _oEnv0.queryId = oraclize_query("URL", _oEnv0.urls[_oEnv0.index]);
          OraclizeEvent(_oEnv0.queryId, _oEnv0.urls[_oEnv0.index]);
        }
      } else if (_queryId == _oEnv1.queryId) {
        _oEnv1.retVals[_oEnv1.index] = _result;
        _oEnv1.index += 1;
        if (_oEnv1.index == 3) {
          processBar(_oEnv1.retVals[0], _oEnv1.retVals[1], _oEnv1.retVals[2]);
          delete _oEnv1; // Reclaim storage costs
        } else {
          _oEnv1.queryId = oraclize_query("URL", _oEnv1.urls[_oEnv1.index]);
          OraclizeEvent(_oEnv1.queryId, _oEnv1.urls[_oEnv1.index]);
        }
      } else if (_queryId == _oEnv2.queryId) {
        processBaz(_result);
        delete _oEnv2;
      } 
    }
}