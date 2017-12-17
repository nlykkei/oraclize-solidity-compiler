''' 
   @file    rand_digraph.py
   @author  Nicolas Lykke Iversen (nlykkei)
   @date    1/1/2018  
   @version 1.0 
   
   @brief Oraclize, Random directed graph generation. 
 
   @section DESCRIPTION
   
   Python script for generating random weighted digraphs.
    - Outputs a weight matrix (uint_max represents infinity)
'''

#!/usr/bin/python3

import sys
import random

def usage():
    print("python3 rand_digraph.py [dim] [uint_max] [self-loop]")
    exit(1)

def main():
    dim = 4
    uint_max = 100
    loop = False
    weight_matrix = []

    if len(sys.argv) > 1:
        dim = int(sys.argv[1])
        if dim <= 0:
            print("\'dim\' must be a positive integer.")
            usage()
    if len(sys.argv) > 2:
        uint_max = int(sys.argv[2])
        if uint_max <= 0:
            print("\'uint_max\' must be a positive integer.")
            usage() 
    if len(sys.argv) > 3:
        if not sys.argv[3] in ["true", "false"]:
            print("\'self-loop\' must be a boolean.")
            usage() 
        loop = (sys.argv[3] == "true")

    weight_matrix = [[random.randint(0, uint_max) for _ in range(dim)] for _ in range(dim)]
    
    if not loop:
    	for i in range(dim):
            weight_matrix[i][i] = uint_max

    j = 0
    web = ""
    remix = "["
    online = ""
    for row in weight_matrix:
        for n in row:
            web += str(n)
            web += "/"

            remix += str(n)
            remix += ","

            online += str(n)
            online += ","

            print('{0: <3}'.format(n), end='  ')
            j += 1  
            if (j % dim) == 0:
                print()
        online = online[:-1]
        online += "\n"
    web = web[:-1]
    remix = remix[:-1] + "]"

    print()
    print(web)
    print()
    print(remix)
    print()
    print(online)

if __name__ == '__main__':
    main()

