set(keximigrate_tsv_PART_SRCS
    TsvMigrate.cpp
    Messages.sh
)

build_and_install_kexi_migrate_driver(tsv
    "${keximigrate_tsv_PART_SRCS}"
    ""
    ""
    ""
)
