Compiling Skia with CMake
=========================

Skia is precompiled (usually via CMake) in both debug and release static libraries.
In order to compile Skia some "adjustments" have to take place:

1) [pending] For linux Jpeg, Gif and Png packages are not needed but they're included by CMake nonetheless,

    #find_package (GIF)
    #find_package (JPEG)
    #find_package (PNG)

Windows doesn't need to exclude these.

2) [pending] By default Skia is configured by CMake as a shared library, make it static:

    add_library (skia STATIC ${srcs})

Remember to update (if needed) the /deps/skia/include directory along with any .lib changes.
    
3) [to be confirmed] It might also be necessary to define the GPU backend

    # GPU backend
    add_definitions(-DSK_SUPPORT_GPU)

Downloading dependencies
========================

Adding a git submodule with the Skia dependency would have been the best choice, however Skia is still
transitioning towards CMake as build configuration system. To shield against changes, code regeneration
and various dependency problems, the easiest way is to provide these precompiled libraries separately.
They are hence stored on github servers as prerelease binaries. They must be downloaded into the deps/
directory before starting the configuration process. This has to be done manually at the moment from the
following URL

    https://github.com/VarcoDevs/Varco/releases/download/v0.1.0-skiadeps/skia.7z

Compression for dependencies
============================

Some dependencies might be compressed with 7zip. They are automatically extracted during
the configuration phase. Compression is done manually when libs/obj files are updated
before checking them in.

Compression for the skia directory (execute this in deps/):
$ 7z a -t7z -m0=lzma -mx=9 skia.7z skia

Decompression might be handled by 7zip if installed and if dependencies are downloaded, otherwise
CMake's 7zip module might be used as a fallback

Decompression:
$ 7z x skia.7z

Updating dependencies
=====================

Update the 7z archives and re-upload them in the github prerelease binaries area.

** OBSOLETE ** Dependencies are in the form of static binaries and can pester git's history. In order to
** OBSOLETE ** update them and remove them from the history (so that only the last one is kept) one could
** OBSOLETE ** run the command
** OBSOLETE ** 
** OBSOLETE **     git filter-branch --force --index-filter \
** OBSOLETE **     'git rm --cached --ignore-unmatch deps/skia/libs/linux/libs.7z' \
** OBSOLETE **     --prune-empty --tag-name-filter cat -- --all
** OBSOLETE ** 
** OBSOLETE ** WARNING: this will also destroy the local copy. After modifications have been done, force push
** OBSOLETE ** the changes to github
** OBSOLETE ** 
** OBSOLETE **     git push origin --force --all
** OBSOLETE ** 
** OBSOLETE ** Collaborators will need to rebase (not merge) if they created branches off the old (tained) repo.
** OBSOLETE ** 
** OBSOLETE ** More info on: https://help.github.com/articles/remove-sensitive-data/
** OBSOLETE ** 
** OBSOLETE ** Using git LFS is recommended.
