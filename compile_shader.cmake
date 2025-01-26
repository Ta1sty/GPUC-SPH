set(source ${SOURCE})
set(output ${OUTPUT})
set(dim ${DIM})


get_filename_component(extension ${source} LAST_EXT)
get_filename_component(output-dir ${output} DIRECTORY)
file(MAKE_DIRECTORY ${output-dir})

set(expanded ${output}.${dim}${extension})
file(REMOVE ${output})
file(REMOVE ${expanded})

execute_process(COMMAND ${GLSLC} -E -DDEF_${dim} -o ${expanded} ${source})

file(READ ${expanded} content)

string(REGEX REPLACE "#line[^\n]*" "" content "${content}")

string(REGEX REPLACE ";" ";\n" content "${content}")
string(REGEX REPLACE "}" "\n}" content "${content}")
string(REGEX REPLACE "{" "{\n" content "${content}")

string(REGEX REPLACE "\n+" "\n" content "${content}")

file(WRITE ${expanded} "${content}")

execute_process(COMMAND ${GLSLC} --target-env=vulkan1.1 -g -o ${output} ${expanded})