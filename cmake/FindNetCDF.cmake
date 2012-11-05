include(FindPackageHandleStandardArgs)

set(paths
  /usr
  /usr/local
)

find_path(NETCDF_INCLUDE_DIR
  NAMES
    netcdf.h
  PATHS
    ${paths}
)

find_library(NETCDF_LIBRARY
  NAMES
    netcdf
  PATHS
    ${paths}
)

find_library(NETCDF_C++_LIBRARY
  NAMES
    netcdf_c++
)

set(NETCDF_LIBRARIES ${NETCDF_LIBRARY} ${NETCDF_C++_LIBRARY})

find_package_handle_standard_args(NETCDF DEFAULT_MSG
  NETCDF_INCLUDE_DIR
  NETCDF_LIBRARIES
)
