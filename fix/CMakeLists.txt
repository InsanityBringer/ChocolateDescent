#add_sources(SOURCE_GROUP "ChocolateDescent\\\\fix"
#    "${METADATA_BASE_PATH}/fix/fix.cpp"
#    "${METADATA_BASE_PATH}/fix/fix.h"
#    "${METADATA_BASE_PATH}/fix/tables.cpp"
#)

target_sources(ChocolateDescent PRIVATE
    "${METADATA_BASE_PATH}/fix/fix.cpp"
    "${METADATA_BASE_PATH}/fix/fix.h"
    "${METADATA_BASE_PATH}/fix/tables.cpp"
)

target_sources(ChocolateDescent2 PRIVATE
    "${METADATA_BASE_PATH}/fix/fix.cpp"
    "${METADATA_BASE_PATH}/fix/fix.h"
    "${METADATA_BASE_PATH}/fix/tables.cpp"
)

