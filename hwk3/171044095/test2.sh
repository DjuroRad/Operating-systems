#! usr/bin/bash
make clear
make all
./makeFileSystem 4 data.data

./fileSystemOper data.data dir "\\"
./fileSystemOper data.data mkdir "\usr"
./fileSystemOper data.data mkdir "\bin"
./fileSystemOper data.data mkdir "\bin\sth"
./fileSystemOper data.data mkdir "\bin\temp"
./fileSystemOper data.data mkdir "\bin\nice"
./fileSystemOper data.data mkdir "\src"

./fileSystemOper data.data dir "\bin"
./fileSystemOper data.data rmdir "\bin\nice"
./fileSystemOper data.data dir "\bin"
./fileSystemOper data.data dir "\\"

./fileSystemOper data.data rmdir "\bin"
./fileSystemOper data.data dir "\bin"
./fileSystemOper data.data dir "\\"