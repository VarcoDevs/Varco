Compiling Skia with CMake
=========================

Skia is precompiled (usually via CMake) in both debug and release static libraries.
In order to compile Skia some "adjustments" have to take place:

1) In skia/cmake/SkUserConfig.h.in it might be necessary for linux systems to define

    #define SK_R32_SHIFT    16
    #define SK_G32_SHIFT    8
    #define SK_B32_SHIFT    0
    #define SK_A32_SHIFT    24

on little-endian systems in order to store bitmaps in kBGRA_8888_SkColorType. This is
in-place for the gyp build (look for SK_SAMPLES_FOR_X), not for the CMake one.

2) Jpeg, Gif and Png packages are not needed but they're included by CMake nonetheless,

    #find_package (GIF)
    #find_package (JPEG)
    #find_package (PNG)

3) By default Skia is configured by CMake as a shared library, make it static:

    add_library (skia STATIC ${srcs})

Remember to update (if needed) the /deps/skia/include directory along with any .lib changes.

Compression for dependencies
============================

Some dependencies might be compressed with 7zip. They are automatically extracted during
the configuration phase. Compression is done manually when libs/obj files are updated
before checking them in.

Compression:
$ 7z a -t7z -m0=lzma -mx=9 libs.7z *.lib *.pdb

Decompression is handled by 7zip if installed, otherwise CMake's 7zip module is used
as a fallback

Decompression:
$ 7z x libs.7z
