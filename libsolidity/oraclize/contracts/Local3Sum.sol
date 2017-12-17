/** 
 *  @file    Local3Sum.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Local "3sum" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking local "3sum".
 */

pragma solidity ^0.4.11;

contract Local3Sum {
    
    //event LogEvent(string msg, uint a, uint b, uint c);

    struct value {
        uint index;
        uint val;
    }

    struct result {
        uint a;
        uint b;
        uint c;
    }

    function Local3Sum() public payable {}    
    
    /// Local 3SUM
    function threeSum(uint[] nums, uint target) public {
        uint n = nums.length;
        value[] memory values = new value[](n);    
        for (uint i = 0; i < n; ++i) {
            values[i] = value({index:i, val: nums[i]});
        }
        quickSort(values, 0, values.length - 1);
        result memory _threeSum = threeSum(values, target);
        //LogEvent("local", _threeSum.a, _threeSum.b, _threeSum.c);
    }

    function threeSum(value[] vals, uint target) private constant returns(result) {
        result memory res;
        uint n = vals.length;
        for (uint i = 0; i <= n - 3; ++i) {
            uint a = vals[i].val;
            uint start = i + 1;
            uint end = n - 1;
            while (start < end) {
                uint b = vals[start].val;
                uint c = vals[end].val;
                if (a + b + c == target) {
                    res.a = vals[i].index;
                    res.b = vals[start].index;
                    res.c = vals[end].index;
                    if (b == vals[start + 1].val) { // Search for all combinations that sum to 'target'
                        start = start + 1;
                    } else {
                        end = end - 1;
                    }
                } else if (a + b + c > target) {
                    end = end - 1;
                } else {
                    start = start + 1;
                }
            }
        }
        return res;
    }

    /// Quicksort
    function quickSort(value[] arr, uint lo, uint hi) private constant {
        if (hi <= lo) return;
        uint j = partition(arr, lo, hi);
        quickSort(arr, lo, j > 0 ? j - 1 : 0);
        quickSort(arr, j + 1, hi);
    }
    
    function partition(value[] arr, uint lo, uint hi) private constant returns(uint) {
        uint i = lo;
        uint j = hi + 1;
        uint pivot = arr[lo].val;
        
        while (true) {
            while (arr[++i].val < pivot) if (i == hi) break;
            while (arr[--j].val > pivot) if (j == lo) break;
            if (i >= j) break;
            swap(arr, i, j);
        }
        swap(arr, lo, j);
        return j;
    }
    
    function swap(value[] arr, uint i, uint j) private constant {
        value memory tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}