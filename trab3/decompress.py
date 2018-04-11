# -*- coding: utf-8 -*-

import sys

def compress(compressed):
    """Decompress a list of output ks to a string."""
    from cStringIO import StringIO
 
    # Build the dictionary.
    dict_size = 256
    dictionary = dict((i, chr(i)) for i in xrange(dict_size))
    # in Python 3: dictionary = {i: chr(i) for i in range(dict_size)}
 
    # use StringIO, otherwise this becomes O(N^2)
    # due to string concatenation in a loop
    result = StringIO()
    w = chr(compressed.pop(0))
    result.write(w)
    for k in compressed:
        if k in dictionary:
            entry = dictionary[k]
        elif k == dict_size:
            entry = w + w[0]
        else:
            raise ValueError('Bad compressed k: %s' % k)
        result.write(entry)
 
        # Add w+entry[0] to the dictionary.
        dictionary[dict_size] = w + entry[0]
        dict_size += 1
 
        w = entry
    return result.getvalue()
 

tocompress = open(sys.argv[1])
print ('compressing...')
compressed = compress(tocompress)
file = open(sys.argv[2])
compressed = compress(compressed)
file.write(compressed)
print ('compressed')