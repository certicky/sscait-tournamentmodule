### SSCAIT Tournament Module DLL

Read the file 'local_files_example/README.txt' for instructions of post-compile copying of the .dll file, and using the SSCAI tournament settings.

The bwapi dependencies must be linked via:

* Solution properties > Configuration Properties > C/C++ > General > Additional Include Directories. This field should contain e.g. "../BWAPI_4.1.2/include".
* Solution properties > Configuration Properties > Linker > Input > Additional Dependencies. This field should contain e.g. "../BWAPI_4.1.2/lib/BWAPI.lib".
