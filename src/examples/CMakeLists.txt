

macro(prepare_proto proto_file)
    get_filename_component(_filename ${proto_file} NAME)
    message(STATUS "Generate: ${_filename}.h from ${proto_file}")
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/${_filename}.h
        COMMAND ${CMAKE_BINARY_DIR}/bin/protopiler --out-dir "${CMAKE_CURRENT_BINARY_DIR}/include"  ${proto_file}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        DEPENDS protopiler ${proto_file}
    )
endmacro()


add_subdirectory(datetime)