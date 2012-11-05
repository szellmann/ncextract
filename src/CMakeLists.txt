find_package(NetCDF)
include_directories(${NETCDF_INCLUDE_DIR})

set(NCEXTRACT_SRC
    main.cpp
)

add_executable(ncextract ${NCEXTRACT_SRC})
target_link_libraries(ncextract ${NETCDF_C++_LIBRARY})

