set(keximigrate_postgresql_PART_SRCS
    PostgresqlMigrate.cpp
    keximigrate_postgresql.json
    Messages.sh
)

build_and_install_kexi_migrate_driver(postgresql
    "${keximigrate_postgresql_PART_SRCS}"
    "${POSTGRESQL_LIBRARIES}"
    "${PostgreSQL_INCLUDE_DIRS}"
    ""
)
