#add_sources(SOURCE_GROUP "ChocolateDescent\\\\vecmat"
#    "${METADATA_BASE_PATH}/vecmat/vecmat.cpp"
#    "${METADATA_BASE_PATH}/vecmat/vecmat.h"
#)

target_sources(ChocolateDescent PRIVATE
    "${METADATA_BASE_PATH}/vecmat/vecmat.cpp"
    "${METADATA_BASE_PATH}/vecmat/vecmat.h"
)

target_sources(ChocolateDescent2 PRIVATE
    "${METADATA_BASE_PATH}/vecmat/vecmat.cpp"
    "${METADATA_BASE_PATH}/vecmat/vecmat.h"
)
