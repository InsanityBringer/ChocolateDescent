#add_sources(SOURCE_GROUP "ChocolateDescent\\\\iff"
#    "${METADATA_BASE_PATH}/iff/iff.cpp"
#    "${METADATA_BASE_PATH}/iff/iff.h"
#)

target_sources(ChocolateDescent PRIVATE
    "${METADATA_BASE_PATH}/iff/iff.cpp"
    "${METADATA_BASE_PATH}/iff/iff.h"
)

target_sources(ChocolateDescent2 PRIVATE
    "${METADATA_BASE_PATH}/iff/iff.cpp"
    "${METADATA_BASE_PATH}/iff/iff.h"
)

