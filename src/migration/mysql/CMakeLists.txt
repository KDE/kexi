set(keximigrate_mysql_PART_SRCS
    mysqlmigrate.cpp
    keximigrate_mysql.json
    Messages.sh
)

build_and_install_kexi_migrate_driver(mysql
    "${keximigrate_mysql_PART_SRCS}"
    "${MYSQL_LIBRARIES}"
    "${MYSQL_INCLUDE_DIR}"
    ""
)
