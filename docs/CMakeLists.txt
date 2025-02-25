find_package(Doxygen REQUIRED)
find_package(Sphinx REQUIRED)

# find remage headers
get_target_property(REMAGE_PUBLIC_HEADER_DIR remage INTERFACE_INCLUDE_DIRECTORIES)
file(GLOB REMAGE_PUBLIC_HEADERS ${REMAGE_PUBLIC_HEADER_DIR}/*.hh)

# configure Doxyfile in build directory
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR})
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

# Doxygen won't create this for us
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

# configure target that runs Doxygen
add_custom_command(
  OUTPUT ${DOXYGEN_INDEX_FILE}
  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT} > /dev/null
  DEPENDS ${DOXYFILE_IN} ${REMAGE_PUBLIC_HEADERS}
  MAIN_DEPENDENCY ${DOXYFILE_OUT})

add_custom_target(
  doxygen ALL
  DEPENDS ${DOXYGEN_INDEX_FILE}
  COMMENT "Generating Doxygen docs")

# Sphinx
set(SPHINX_SOURCE ${CMAKE_CURRENT_BINARY_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/_build)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

# FIXME: the logic here to handle sources is not really clean, needs some more
# serious thought! one important requirement is CMake being able to copy the
# sources again if they change
file(
  GLOB SPHINX_SOURCES
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  *.rst *.md)

foreach(_file ${SPHINX_SOURCES})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${_file} ${SPHINX_SOURCE} COPYONLY)
endforeach()

file(
  GLOB SPHINX_IMAGES
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  img/*)

file(MAKE_DIRECTORY ${SPHINX_SOURCE}/img)
foreach(_file ${SPHINX_IMAGES})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${_file} ${SPHINX_SOURCE}/img COPYONLY)
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in ${SPHINX_SOURCE}/conf.py @ONLY)

add_custom_command(
  OUTPUT ${SPHINX_INDEX_FILE}
  COMMAND ${SPHINX_EXECUTABLE} -q -b html -Dbreathe_projects.remage=${DOXYGEN_OUTPUT_DIR}/xml
          ${SPHINX_SOURCE} ${SPHINX_BUILD}
  WORKING_DIRECTORY ${SPHINX_SOURCE}
  DEPENDS ${SPHINX_SOURCES} ${SPHINX_IMAGES} ${DOXYGEN_INDEX_FILE}
  MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
  ${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in)

# Nice named target so we can run the job easily
add_custom_target(
  sphinx ALL
  DEPENDS ${SPHINX_INDEX_FILE}
  COMMENT "Generating Sphinx docs")

# Add an install target to install the docs
include(GNUInstallDirs)
install(DIRECTORY ${SPHINX_BUILD} DESTINATION ${CMAKE_INSTALL_DOCDIR})
