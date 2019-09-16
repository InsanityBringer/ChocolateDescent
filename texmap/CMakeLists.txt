#add_sources(SOURCE_GROUP "ChocolateDescent\\\\texmap"
#    "${METADATA_BASE_PATH}/texmap/ntmap.cpp"
#    "${METADATA_BASE_PATH}/texmap/scanline.cpp"
#    "${METADATA_BASE_PATH}/texmap/scanline.h"
#    "${METADATA_BASE_PATH}/texmap/texmap.h"
#    "${METADATA_BASE_PATH}/texmap/texmapl.h"
#    "${METADATA_BASE_PATH}/texmap/tmapflat.cpp"
#    "${METADATA_BASE_PATH}/texmap/tmapsky.cpp"
#)

target_sources(ChocolateDescent PRIVATE
    "${METADATA_BASE_PATH}/texmap/ntmap.cpp"
    "${METADATA_BASE_PATH}/texmap/scanline.cpp"
    "${METADATA_BASE_PATH}/texmap/scanline.h"
    "${METADATA_BASE_PATH}/texmap/texmap.h"
    "${METADATA_BASE_PATH}/texmap/texmapl.h"
    "${METADATA_BASE_PATH}/texmap/tmapflat.cpp"
    "${METADATA_BASE_PATH}/texmap/tmapsky.cpp"
)

target_sources(ChocolateDescent2 PRIVATE
    "${METADATA_BASE_PATH}/texmap/ntmap_d2.cpp"
    "${METADATA_BASE_PATH}/texmap/ntmapout.h"
    "${METADATA_BASE_PATH}/texmap/scanline_d2.cpp"
    "${METADATA_BASE_PATH}/texmap/scanline.h"
    "${METADATA_BASE_PATH}/texmap/texmap.h"
    "${METADATA_BASE_PATH}/texmap/texmapl.h"
    "${METADATA_BASE_PATH}/texmap/tmapflat_d2.cpp"
#    "${METADATA_BASE_PATH}/texmap/tmapsky.cpp" #[ISB] I don't know what the dream is, but I want to fill it sometime
)
