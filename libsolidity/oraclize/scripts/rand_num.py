''' 
   @file    rand_num.py
   @author  Nicolas Lykke Iversen (nlykkei)
   @date    1/1/2018  
   @version 1.0 
   
   @brief Oraclize, Random number generation. 
 
   @section DESCRIPTION
   
   Python script for generating random n-digit numbers.
'''

#!/usr/bin/python3

import sys
import random

def usage():
    print("python3 rand_array.py [digits]")
    exit(1)

def random_with_N_digits(n):
    range_start = 10 ** (n - 1)
    range_end = (10 ** n) - 1
    return random.randint(range_start, range_end)

def main():
    digits = 10

    if len(sys.argv) > 1:
        digits = int(sys.argv[1])
        if digits <= 0:
            print("\'digits\' must be a positive integer.")
            usage()

    print(random_with_N_digits(digits))

if __name__ == '__main__':
    main()

