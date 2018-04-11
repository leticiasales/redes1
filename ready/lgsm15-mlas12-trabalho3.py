from sys import argv

_, filename = argv

def initDic(dic):
    for num in range(0,128):
        dic.append(chr(num))
    return dic

def compress(filename):
    file = open(filename, 'r')
    text = file.read()
    current = ""
    maxSize = 32767
    compText = []
    dic = []
    dic = initDic(dic)

    for c in text:
        toAdd = current + c
        if toAdd in dic:
            current = toAdd
        else:
            #print "I",dic.index(current),"I", "\n"
            if len(dic) < maxSize:
                dic.append(toAdd)
            compText.append(dic.index(current))
            current = c

    # adiciona ultimo elemento
    if current:
        compText.append(dic.index(current))

    outFile = open(name+".lzw16", 'w')
    for val in compText:
        #print >> outFile, val
        outFile.write(str(val)+":")
    #print chr(compText.pop(0))

def decompress(filename):
    file = open(filename, 'r')
    text = file.read()
    dic = []
    dic = initDic(dic)
    pos = text.split(":")
    del pos[-1]                 # remove ultimo elemento ""
    decompText = []

    w = dic[int(pos.pop(0))]
    decompText.append(w)
    for k in pos:
        k = int(k)
        if k < len(dic):
            entry = dic[k]
        else:
            entry = w + w[0]
        decompText.append(entry)
 
        # Add w+entry[0] to the dictionary.
        dic.append(w + entry[0]) 
        w = entry

    outFile = open(name+"Dec.txt", 'w')
    for val in decompText:
        #print >> outFile, val
        outFile.write(str(val))

name, form = filename.split(".")

if form == "lzw16":
    decompress(filename)
else:
    compress(filename)