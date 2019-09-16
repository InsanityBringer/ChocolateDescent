#add_sources(SOURCE_GROUP "ChocolateDescent\\\\misc"
#    "${METADATA_BASE_PATH}/misc/byteswap.cpp"
#    "${METADATA_BASE_PATH}/misc/byteswap.h"
#    "${METADATA_BASE_PATH}/misc/error.cpp"
#    "${METADATA_BASE_PATH}/misc/error.h"
#    "${METADATA_BASE_PATH}/misc/parsarg.cpp"
#    "${METADATA_BASE_PATH}/misc/parsarg.h"
#    "${METADATA_BASE_PATH}/misc/types.h"
#)

target_sources(ChocolateDescent PRIVATE
    "${METADATA_BASE_PATH}/misc/byteswap.cpp"
    "${METADATA_BASE_PATH}/misc/byteswap.h"
    "${METADATA_BASE_PATH}/misc/error.cpp"
    "${METADATA_BASE_PATH}/misc/error.h"
    "${METADATA_BASE_PATH}/misc/parsarg.cpp"
    "${METADATA_BASE_PATH}/misc/parsarg.h"
    "${METADATA_BASE_PATH}/misc/types.h"
)

target_sources(ChocolateDescent2 PRIVATE
    "${METADATA_BASE_PATH}/misc/args.cpp"
    "${METADATA_BASE_PATH}/misc/args.h"
    "${METADATA_BASE_PATH}/misc/byteswap.cpp"
    "${METADATA_BASE_PATH}/misc/byteswap.h"
    "${METADATA_BASE_PATH}/misc/error.cpp"
    "${METADATA_BASE_PATH}/misc/error.h"
    "${METADATA_BASE_PATH}/misc/hash.cpp"
    "${METADATA_BASE_PATH}/misc/hash.h"
    "${METADATA_BASE_PATH}/misc/parsarg.cpp"
    "${METADATA_BASE_PATH}/misc/parsarg.h"
    "${METADATA_BASE_PATH}/misc/types.h"
)

