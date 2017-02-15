

execute_process(
    COMMAND ${POSTGRESQL_PG_CONFIG} --sharedir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE SHARE_DIR)

if(SHARE_DIR)
    set(SHARE_DIR "${SHARE_DIR}/extension")
    message(STATUS "Extension directory for SQL files is set to ${SHARE_DIR}")
else(SHARE_DIR)
    message(FATAL_ERROR "pg_config --sharedir failed to return a value. Please check your PostgreSQL installation!")
endif(SHARE_DIR)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/pgtempus/pgtempus--1.0.sql"
    "${CMAKE_CURRENT_SOURCE_DIR}/pgtempus/pgtempus.control"
DESTINATION "${SHARE_DIR}")
