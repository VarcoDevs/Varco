#
# Find 7Zip on the system, the following variables will be set:
#
# SEVENZIP_BINARY - the executable
# SEVENZIP_FOUND  - true if found
#
include (FindPackageHandleStandardArgs)

if (SEVENZIP_BINARY)
	set (SEVENZIP_FIND_QUIETLY TRUE) # Already provided/in cache, be silent if you can't find it
else()
  message (STATUS "Searching for 7Zip on the system (required to unpack binary dependencies)")
endif (SEVENZIP_BINARY)

find_program (SEVENZIP_BINARY
	NAMES 7z 7za
	HINTS "$ENV{ProgramFiles\(x86\)}/7-Zip" "$ENV{ProgramFiles}/7-Zip" "${MINGWDIR}" "${MINGWLIBS}/bin"
	PATH_SUFFIXES bin
	DOC "7Zip binary executable")

# Handle QUIETLY and REQUIRED arguments and set SEVENZIP_FOUND to TRUE if found
find_package_handle_standard_args(SevenZip DEFAULT_MSG SEVENZIP_BINARY)

mark_as_advanced (SEVENZIP_BINARY)