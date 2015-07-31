# Copyright (c) 2015 Andrew Kelley
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# DW_FOUND
# DW_INCLUDE_DIR
# DW_LIBRARY

find_path(DW_INCLUDE_DIR NAMES elfutils/libdwfl.h)

find_library(DW_LIBRARY NAMES dw)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DW DEFAULT_MSG DW_LIBRARY DW_INCLUDE_DIR)

mark_as_advanced(DW_INCLUDE_DIR DW_LIBRARY)

