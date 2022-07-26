#! usr/bin/bash
make clear
make all
./makeFileSystem 4 data.data

./fileSystemOper data.data dir "\\"
./fileSystemOper data.data mkdir "\usr"
./fileSystemOper data.data mkdir "\bin"
./fileSystemOper data.data mkdir "\src"

./fileSystemOper data.data mkdir "\temp"
./fileSystemOper data.data mkdir "\temp\nesto"
./fileSystemOper data.data dir "\temp\nesto"

./fileSystemOper data.data mkdir "\temp2"
./fileSystemOper data.data mkdir "\temp3"

./fileSystemOper data.data mkdir "\temp2\radi"
./fileSystemOper data.data mkdir "\temp2\radidobro"
./fileSystemOper data.data mkdir "\temp2\radi"

./fileSystemOper data.data mkdir "\temp2\radistrasno"
./fileSystemOper data.data dir "\temp2"
./fileSystemOper data.data dir "\temp2\radidobro"

./fileSystemOper data.data dir "\temp2\."
./fileSystemOper data.data dir "\temp2\.."