# Copyright (c) 2015 Andrew Kelley
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# UNWIND_FOUND
# UNWIND_INCLUDE_DIR
# UNWIND_LIBRARY

find_path(UNWIND_INCLUDE_DIR NAMES libunwind.h)

find_library(UNWIND_LIBRARY NAMES unwind)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UNWIND DEFAULT_MSG UNWIND_LIBRARY UNWIND_INCLUDE_DIR)

mark_as_advanced(UNWIND_INCLUDE_DIR UNWIND_LIBRARY)
