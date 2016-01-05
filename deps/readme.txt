Some dependencies might be compressed with 7zip. They are automatically extracted during
the configuration phase. Compression is done manually when libs/obj files are updated
before checking them in.

Compression:
$ 7z a -t7z -m0=lzma -mx=9 libs.7z *.lib *.pdb

Decompression is handled by 7zip if installed, otherwise CMake's 7zip module is used
as a fallback

Decompression:
$ 7z x libs.7z