''' 
   @file    rand_graph.py
   @author  Nicolas Lykke Iversen (nlykkei)
   @date    1/1/2018  
   @version 1.0 
   
   @brief Oraclize, Random undirected graph generation. 
 
   @section DESCRIPTION
   
   Python script for generating random undirected graphs.
    - Outputs an adjacency matrix.
'''

#!/usr/bin/python3

import sys
import random

def usage():
    print("python3 rand_graph.py [dim] [self-loop]")
    exit(1)

def main():
    dim = 4
    loop = False
    adj_matrix = []

    if len(sys.argv) > 1:
        dim = int(sys.argv[1])
        if dim <= 0:
            print("\'dim\' must be a positive integer.")
            usage()
    if len(sys.argv) > 2:
        if not sys.argv[2] in ["true", "false"]:
            print("\'self-loop\' must be a boolean.")
            usage() 
        loop = (sys.argv[2] == "true")

    adj_matrix = [[0 for _ in range(dim)] for _ in range(dim)]

    for i in range(dim): # (0,0), (1,0), (1,1), (2,0), (2,1), (2,2),..., (dim-1, 0),..., (dim-1, dim-1)
        for j in range(i + 1):
            entry = random.randint(0, 1)
            adj_matrix[i][j] = adj_matrix[j][i] = entry

    if not loop:
    	for i in range(dim):
            adj_matrix[i][i] = 0

    j = 0
    web = ""
    remix = "["
    online = ""
    for row in adj_matrix:
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

