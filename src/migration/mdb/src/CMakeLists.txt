if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUC)
    add_definitions(-Wno-unused-but-set-variable)
    add_definitions(-Wno-missing-format-attribute)
    add_definitions(-Wno-sign-compare)
    add_definitions(-Wno-unused-parameter)
    add_definitions(-Wno-pointer-arith)
    add_definitions(-std=c99)
    add_definitions(-Wno-cast-qual)
endif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUC)

set(MDB_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/migration/mdb)
set(MDBTOOLS_SOURCE_DIR ${MDB_SOURCE_DIR}/3rdparty/mdbtools)
set(keximigrate_mdb_INCLUDES ${GLIB2_INCLUDE_DIR} ${ICONV_INCLUDE_DIR} ${MDBTOOLS_SOURCE_DIR}/include)
set(keximigrate_mdb_LIBRARIES ${GLIB2_LIBRARIES} ${ICONV_LIBRARIES})

set(keximigrate_mdb_PART_SRCS
keximdb/mdbmigrate.cpp
keximdb/keximigrate_mdb.json
${MDBTOOLS_SOURCE_DIR}/libmdb/catalog.c
${MDBTOOLS_SOURCE_DIR}/libmdb/data.c
${MDBTOOLS_SOURCE_DIR}/libmdb/dump.c
${MDBTOOLS_SOURCE_DIR}/libmdb/file.c
${MDBTOOLS_SOURCE_DIR}/libmdb/iconv.c
${MDBTOOLS_SOURCE_DIR}/libmdb/index.c
${MDBTOOLS_SOURCE_DIR}/libmdb/like.c
${MDBTOOLS_SOURCE_DIR}/libmdb/map.c
${MDBTOOLS_SOURCE_DIR}/libmdb/mem.c
${MDBTOOLS_SOURCE_DIR}/libmdb/money.c
${MDBTOOLS_SOURCE_DIR}/libmdb/options.c
${MDBTOOLS_SOURCE_DIR}/libmdb/props.c
${MDBTOOLS_SOURCE_DIR}/libmdb/sargs.c
${MDBTOOLS_SOURCE_DIR}/libmdb/stats.c
${MDBTOOLS_SOURCE_DIR}/libmdb/table.c
${MDBTOOLS_SOURCE_DIR}/libmdb/worktable.c
${MDBTOOLS_SOURCE_DIR}/libmdb/write.c
${MDB_SOURCE_DIR}/Messages.sh
)

build_and_install_kexi_migrate_driver(mdb
    "${keximigrate_mdb_PART_SRCS}"
    "${keximigrate_mdb_LIBRARIES}"
    "${keximigrate_mdb_INCLUDES}"
    "-DHAVE_ICONV=1"
)
