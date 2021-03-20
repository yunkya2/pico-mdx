#!/usr/bin/env python3
import sys
import os
import re

# headers for MXDRVG

mdxheader_pdx   = b'\x00\x00\x00\x00\x00\x0a\x00\x08\x00\x00'
mdxheader_nopdx = b'\x00\x00\xff\xff\x00\x0a\x00\x08\x00\x00'
pdxheader       = b'\x00\x00\x00\x00\x00\x0a\x00\x02\x00\x00'

pdxpath = []
mdxpdxinfo = []
mdxdata = []
pdxdata = []
pdxfilelist = []

def getwinfile(path):
    dir, file = os.path.split(path)
    for f in os.listdir(dir):
        realfile = os.path.join(dir, f)
        if os.path.isfile(realfile) and f.lower() == file.lower():
            return realfile
    return None

def findpdxfile(mdxname, pdxname):
    if not pdxname:
        return None

    if not pdxname.lower().endswith('.pdx'):
        pdxname += '.pdx'

    for p in [os.path.dirname(mdxname)] + pdxpath:
        pdxfile = p + '/' + pdxname
        res = getwinfile(pdxfile)
        if res:
            return res

    return None

def dumpdata(name, data):
    out = name + ':\n'
    p = 0
    while len(data[p:]):
        out += '.byte 0x%02x' % data[p]
        for c in data[p + 1:p + 16]:
            out += ', 0x%02x' % c
        p += 16
        out += '\n'
    return out

def makemdxdata(cwd, infile, outfile):
    with open(infile) as f:
        while True:
            line = f.readline()
            if not line:
                break
            line = line.strip()
            if line.startswith('#') or line == '':
                continue

            if line.startswith('%'):
                path = line[1:]
                if not path.startswith('/'):
                    path = cwd + '/' + path
                pdxpath.append(path)
                continue
        
            if not line.startswith('/'):
                line = cwd + '/' + line

            print("mdx: %s" % line)
            with open(line, 'rb') as fm:
                data = fm.read()
    
                p1 = data.find(b'\x1a')
                p2 = data.find(b'\0', p1 + 1)
                pdxname = data[p1 + 1:p2].decode()
    
                pdxfile = findpdxfile(line, pdxname)
                if pdxfile:
                    if pdxfile in pdxfilelist:
                        mdxpdxinfo.append(pdxfilelist.index(pdxfile))
                    else:
                        print("pdx: %s" % pdxfile)
                        with open(pdxfile, 'rb') as fp:
                            pdxfilelist.append(pdxfile)
                            mdxpdxinfo.append(len(pdxdata))
                            pdxdata.append(pdxheader + fp.read())
                    mdxdata.append(mdxheader_pdx + data[p2 + 1:])
                else:
                    mdxpdxinfo.append(-1)
                    mdxdata.append(mdxheader_nopdx + data[p2 + 1:])

    with open(outfile, 'w') as f:
        for m in zip(range(len(mdxdata)), mdxdata, mdxpdxinfo):
            f.write('.word mdxdata%d, %d, ' % (m[0], len(m[1])))
            if m[2] >= 0:
                f.write('pdxdata%d, %d\n' % (m[2], len(pdxdata[m[2]])))
            else:
                f.write('0, 0\n')
        f.write('.word 0\n')
        for m in zip(range(len(mdxdata)), mdxdata):
            f.write(dumpdata("mdxdata%d" % m[0], m[1]))
#        for p in zip(range(len(pdxdata)), pdxdata):
#            f.write(dumpdata("pdxdata%d" % p[0], p[1]))
        for p in zip(range(len(pdxfilelist)), pdxfilelist):
            f.write("pdxdata%d:\n" % p[0])
            f.write('.incbin "' + p[1] +'"\n')

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: %s <mdxlist.txt> <mdxdata.inc>" % sys.argv[0])
        sys.exit(1)

    makemdxdata(os.path.dirname(sys.argv[0]), sys.argv[1], sys.argv[2])
