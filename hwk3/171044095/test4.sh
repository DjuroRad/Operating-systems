#! usr/bin/bash
make clear
make all
./makeFileSystem 1 fileSystem.data

./fileSystemOper fileSystem.data mkdir "\usr"
./fileSystemOper fileSystem.data mkdir "\usr\ysa"
./fileSystemOper fileSystem.data mkdir "\bin\ysa"
./fileSystemOper fileSystem.data write "\usr\ysa\file1" linuxFile.a
./fileSystemOper fileSystem.data write "\usr\file2" linuxFile.a
./fileSystemOper fileSystem.data write "\file3" linuxFile.a

./fileSystemOper fileSystem.data dir "\\"
./fileSystemOper fileSystem.data del "\usr\ysa\file1"
./fileSystemOper fileSystem.data dumpe2fs
./fileSystemOper fileSystem.data read "\usr\file2" linuxFile2.a
cmp linuxFile2.a linuxFile.a