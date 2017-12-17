/** 
 *  @file    LocalSort.sol
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Local "sort" benchmark. 
 *
 *  @section DESCRIPTION
 *  
 *  Contract for benchmarking local "sort".
 */

pragma solidity ^0.4.11;

contract LocalSort {
    
    //event LogEvent(string msg, uint[] res);

    function LocalSort() public payable {}    
    
    /// Local sort (quicksort)
    function sort(uint[] nums) public {
        if (nums.length > 1)  {
            quickSort(nums, 0, nums.length - 1);
        }
        //LogEvent("local", nums);
    }

    /// Quicksort
    function quickSort(uint[] arr, uint lo, uint hi) private constant {
        if (hi <= lo) return;
        uint j = partition(arr, lo, hi);
        quickSort(arr, lo, j > 0 ? j - 1 : 0);
        quickSort(arr, j + 1, hi);
    }
    
    function partition(uint[] arr, uint lo, uint hi) private constant returns(uint) {
        uint i = lo;
        uint j = hi + 1;
        uint pivot = arr[lo];
        
        while (true) {
            while (arr[++i] < pivot) if (i == hi) break;
            while (arr[--j] > pivot) if (j == lo) break;
            if (i >= j) break;
            swap(arr, i, j);
        }
        swap(arr, lo, j);
        return j;
    }
    
    function swap(uint[] arr, uint i, uint j) private constant {
        uint tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }      
}