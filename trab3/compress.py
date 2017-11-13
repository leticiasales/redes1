# -*- coding: utf-8 -*-
import sys

dictionary = []

def compress(input_string):
    global dictionary
    for char in input_string:
        if char in dictionary:  
            continue
        else:
            dictionary.append(char)
    dictionary.sort()
    no_input = len(dictionary)
    next = ""
    for char in input_string:
        a = next + char
        if a in dictionary:
            next = a
        else:
            next = a[len(a)-1]
            dictionary_word = a[0:len(a)-1] 
            dictionary.append(a)
            encoded_version.append(dictionary.index(dictionary_word))
            encoded_version.append(dictionary.index(next))

tocompress = open(sys.argv[1], 'r')
print ('compressing...')
compressed = compress(tocompress)
file = open(sys.argv[2], 'w')
file.write(compressed)
print ('compressed')