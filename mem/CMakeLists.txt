#add_sources(SOURCE_GROUP "ChocolateDescent\\\\mem"
#    "${METADATA_BASE_PATH}/mem/mem.cpp"
#    "${METADATA_BASE_PATH}/mem/mem.h"
#)

target_sources(ChocolateDescent PRIVATE
    "${METADATA_BASE_PATH}/mem/mem.cpp"
    "${METADATA_BASE_PATH}/mem/mem.h"
)

target_sources(ChocolateDescent2 PRIVATE
    "${METADATA_BASE_PATH}/mem/mem.cpp"
    "${METADATA_BASE_PATH}/mem/mem.h"
)

