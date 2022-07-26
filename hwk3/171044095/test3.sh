#! usr/bin/bash
make clear
make all
./makeFileSystem 1 data.data

./fileSystemOper data.data dir "\\"
./fileSystemOper data.data mkdir "\usr"
./fileSystemOper data.data mkdir "\bin"
./fileSystemOper data.data mkdir "\bin\sth"
./fileSystemOper data.data mkdir "\bin\temp"
./fileSystemOper data.data mkdir "\bin\nice"
./fileSystemOper data.data write "\bin\works" somefile.txt
./fileSystemOper data.data read "\bin\works" thisWorks.txt
./fileSystemOper data.data write "\bin\works2" somefile2.txt
./fileSystemOper data.data read "\bin\works2" thisWorks2.txt
./fileSystemOper data.data dir "\bin"
cmp thisWorks.txt somefile.txt
cmp thisWorks2.txt somefile2.txt
./fileSystemOper data.data dumpe2fs