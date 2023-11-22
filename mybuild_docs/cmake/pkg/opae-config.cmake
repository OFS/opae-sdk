get_filename_component(_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_prefix "${_dir}/../.." ABSOLUTE)

# import targets
include("${_prefix}/lib/opae-2.10.0/opae-targets.cmake")
set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	"${_prefix}/lib/opae-2.10.0/modules"
)

set(opae_INCLUDE_DIRS "${_prefix}/include")
