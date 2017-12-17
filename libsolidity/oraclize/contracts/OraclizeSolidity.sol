/** 
 *  @file    OraclizeSolidity.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Library of utility functions for oraclizing contracts. 
 *
 *  @section DESCRIPTION
 *  
 *  Library of utility functions to be used with Oraclize compiler pass.
 *      - Oraclized contracts should inherit from this base contract.
 */

pragma solidity ^0.4.11;

import "./oraclizeAPI.sol";
import "./strings.min.sol";

contract OraclizeSolidity is usingOraclize {

    using strings for *;   
    
    /// Converts uint array into string with uints separated by 'delim'   
    function arrayToString(uint[] arr, string delim) internal constant returns (string) {
        strings.slice[] memory slices = new strings.slice[](arr.length);
        for (uint i = 0; i < arr.length; i++) {
            slices[i] = uintToBytes(arr[i]).toSliceB32();
        }
        return delim.toSlice().join(slices);
    }

    /// Converts binary string of (16-bit) uints into uint array
    function stringToArray(string nums) internal constant returns(uint[]) {
        bytes memory bnums = bytes(nums);
        uint[] memory parts = new uint[](bnums.length / 2);
        for (uint i = 0; i < parts.length; ++i) {
            parts[i] += (uint(bnums[2 * i]) << 8);
            parts[i] += (uint(bnums[2 * i + 1]) << 0);
        }
        return parts;
    }

    /// Converts uint into string prefixed by 'prefix'
    function uintToStringWithPrefix(uint num, string prefix) internal constant returns (string) {
        return prefix.toSlice().concat(uint2str(num).toSlice());
    }   

    /// Converts uint array into string with uints separated by 'delim' and prefixed by 'prefix'.
    function arrayToStringWithPrefix(uint[] arr, string delim, string prefix) internal constant returns (string) {
        strings.slice[] memory slices = new strings.slice[](arr.length);
        for (uint i = 0; i < arr.length; i++) {
            slices[i] = uintToBytes(arr[i]).toSliceB32();
        }
        return prefix.toSlice().concat(delim.toSlice().join(slices).toSlice());
    }

    /// Converts uint and uint8 array into string with uints separated by 'delim' and prefixed by 'prefix'
    function uintAndArrayToStringWithPrefix(uint num, uint8[] arr, string delim, string prefix) internal constant returns (string) {
        uint _len = arr.length;
        uint[] memory _arr = new uint[](_len + 1);
        _arr[0] = num;
        for (uint i = 0; i < _len; ++i) {
            _arr[i + 1] = arr[i];
        }
        return arrayToStringWithPrefix(_arr, delim, prefix);
    }  

    /// Converts uint and uint16 array into string with uints separated by 'delim' and prefixed by 'prefix'
    function uintAndArrayToStringWithPrefix(uint num, uint16[] arr, string delim, string prefix) internal constant returns (string) {
        uint _len = arr.length;
        uint[] memory _arr = new uint[](_len + 1);
        _arr[0] = num;
        for (uint i = 0; i < _len; ++i) {
            _arr[i + 1] = arr[i];
        }
        return arrayToStringWithPrefix(_arr, delim, prefix);
    }     

    /// Converts uint and uint array into string with uints separated by 'delim' and prefixed by 'prefix'
    function uintAndArrayToStringWithPrefix(uint num, uint[] arr, string delim, string prefix) internal constant returns (string) {
        uint _len = arr.length;
        uint[] memory _arr = new uint[](_len + 1);
        _arr[0] = num;
        for (uint i = 0; i < _len; ++i) {
            _arr[i + 1] = arr[i];
        }
        return arrayToStringWithPrefix(_arr, delim, prefix);
    }    

    /// Converts two uint16 and uint16 array into string with uints separated by 'delim' and prefixed by 'prefix'
    function uintsAndArrayToStringWithPrefix(uint num1, uint num2, uint16[] arr, string delim, string prefix) internal constant returns (string) {
        uint _len = arr.length;
        uint[] memory _arr = new uint[](_len + 2);
        _arr[0] = num1;
        _arr[1] = num2;
        for (uint i = 0; i < _len; ++i) { 
            _arr[i + 2] = arr[i];
        }
        return arrayToStringWithPrefix(_arr, delim, prefix);
    }   

    /// Converts two uint and uint array into string with uints separated by 'delim' and prefixed by 'prefix'
    function uintsAndArrayToStringWithPrefix(uint num1, uint num2, uint[] arr, string delim, string prefix) internal constant returns (string) {
        uint _len = arr.length;
        uint[] memory _arr = new uint[](_len + 2);
        _arr[0] = num1;
        _arr[1] = num2;
        for (uint i = 0; i < _len; ++i) { 
            _arr[i + 2] = arr[i];
        }
        return arrayToStringWithPrefix(_arr, delim, prefix);
    }   
 
    function parseInt(string _a, uint _b) internal returns (uint) {
        bytes memory bresult = bytes(_a);
        uint mint = 0;
        bool decimals = false;
        for (uint i=0; i<bresult.length; i++) {
            if ((bresult[i] >= 48)&&(bresult[i] <= 57)){
                if (decimals){
                   if (_b == 0) break;
                    else _b--;
                }
                mint *= 10;
                mint += uint(bresult[i]) - 48;
            } else if (bresult[i] == 46) decimals = true;
        }
        if (_b > 0) mint *= 10**_b;
        return mint;
    }

    /// Converts uint to string (oraclizeAPI)
    /// function uint2str(uint i) private returns (string) 
    
    /// Converts uint to bytes32.
    function uintToBytes(uint v) private constant returns (bytes32 ret) {
        if (v == 0) {
            ret = '0';
        }
        else {
            while (v > 0) {
                ret = bytes32(uint(ret) / (2 ** 8));
                ret |= bytes32(((v % 10) + 48) * 2 ** (8 * 31));
                v /= 10;
            }
        }
        return ret;
    }           
    
    /// Babylonian square root
    function babylonian(uint n) internal constant returns(uint) {
        uint x = n;
        uint y = 1;
        while (x > y) {
            x = (x + y) / 2;
            y = n / x;
        }
        return x;
    }

    /// Unpacks string (2 bytes) to uint
    function unpack16Bit(string pack) internal returns (uint) {
        uint res = 0;
        bytes memory bres = bytes(pack);
        if (bres.length != 2) {
            return 0;
        }
        res += uint(bres[0] << 8);
        res += uint(bres[1] << 0);
        return res;
    }
}