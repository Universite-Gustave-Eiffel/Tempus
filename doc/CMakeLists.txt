find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/api/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/api/Doxyfile @ONLY)
    add_custom_target(doc ALL
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/api/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/api/html DESTINATION ${TEMPUS_INSTALL_DOC_DIRECTORY}/api)
endif()

install(DIRECTORY user DESTINATION ${TEMPUS_INSTALL_DOC_DIRECTORY})
install(DIRECTORY images DESTINATION ${TEMPUS_INSTALL_DOC_DIRECTORY})
install(FILES README.md README_loader.md README_WIN32.md README_BDAlti.md nantes_data.md bordeaux_data.md Installation.md Documentation.md DESTINATION ${TEMPUS_INSTALL_DOC_DIRECTORY})