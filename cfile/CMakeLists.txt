#add_sources(SOURCE_GROUP "ChocolateDescent\\\\cfile"
#    "${METADATA_BASE_PATH}/cfile/cfile.cpp"
#    "${METADATA_BASE_PATH}/cfile/cfile.h"
#)

target_sources(ChocolateDescent PRIVATE
    "${METADATA_BASE_PATH}/cfile/cfile.cpp"
    "${METADATA_BASE_PATH}/cfile/cfile.h"
)

target_sources(ChocolateDescent2 PRIVATE
    "${METADATA_BASE_PATH}/cfile/cfile.cpp"
    "${METADATA_BASE_PATH}/cfile/cfile.h"
)
