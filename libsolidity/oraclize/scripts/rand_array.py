''' 
   @file    rand_array.py
   @author  Nicolas Lykke Iversen (nlykkei)
   @date    1/1/2018  
   @version 1.0 
   
   @brief Oraclize, Random array generation. 
 
   @section DESCRIPTION
   
   Python script for generating random arrays of integers.
'''

#!/usr/bin/python3

import sys
import random

def usage():
    print("python3 rand_array.py [array_size] [uint_max]")
    exit(1)

def main():
    array_size = 10
    uint_max = 100
    arr = []

    if len(sys.argv) > 1:
        array_size = int(sys.argv[1])
        if array_size <= 0:
            print("\'array_size\' must be a positive integer.")
            usage()
    if len(sys.argv) > 2:
        uint_max = int(sys.argv[2])
        if uint_max <= 0:
            print("\'uint_max\' must be a positive integer.")
            usage() 

    for _ in range(array_size):
        arr.append(random.randint(0, uint_max))
    
    print(arr)

if __name__ == '__main__':
    main()

