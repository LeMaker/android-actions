:: requires Microsoft Visual C++, run vcvars32 to set env first
cl src/main.c src/md5c.c src/xpt-b64.c  /I ./hdr /o ./bin/servercred.exe

