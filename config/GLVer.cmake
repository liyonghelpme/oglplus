#  Copyright 2010-2013 Matus Chochlik. Distributed under the Boost
#  Software License, Version 1.0. (See accompanying file
#  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

macro(gl_version_detection GL_LIB VERSION)
	set(OGLPLUS_CONFIG_QUERY_GL_VER ${GL_LIB}_VERSION_${VERSION})
	string(REPLACE "_" ";" GL_VER_LIST ${VERSION})
	list(GET GL_VER_LIST 0 OGLPLUS_CONFIG_QUERY_GL_VER_MAJOR)
	list(GET GL_VER_LIST 1 OGLPLUS_CONFIG_QUERY_GL_VER_MINOR)

	if(NOT DEFINED OGLPLUS_CONFIG_STRICT_VERSION_CHECK)
		set(OGLPLUS_CONFIG_STRICT_VERSION_CHECK 1)
	endif()

	gl_feature_detection(${GL_LIB} ${VERSION} VER)

	unset(OGLPLUS_CONFIG_QUERY_GL_VER)
	unset(OGLPLUS_CONFIG_QUERY_GL_VER_MAJOR)
	unset(OGLPLUS_CONFIG_QUERY_GL_VER_MINOR)
endmacro()

if(NOT OGLPLUS_MAX_GL_VERSION)
	set(OGLPLUS_MAX_GL_VERSION 9_9)
endif()

message(STATUS "Detecting OpenGL version")
foreach(GL_VERSION 4_4 4_3 4_2 4_1 4_0 3_3 3_2 3_1)
	if(NOT("${GL_VERSION}" STRGREATER "${OGLPLUS_MAX_GL_VERSION}"))
		gl_version_detection(GL ${GL_VERSION})
		if(${HAS_GL_${GL_VERSION}})
			string(REPLACE "_" ";" GL_VER_LIST ${GL_VERSION})
			list(GET GL_VER_LIST 0 OGLPLUS_GL_VERSION_MAJOR)
			list(GET GL_VER_LIST 1 OGLPLUS_GL_VERSION_MINOR)
			break()
		endif()
	endif()
	set(OGLPLUS_NO_GL_VERSION_${GL_VERSION} true)
endforeach()

if(OGLPLUS_GL_VERSION_MAJOR)
	message(STATUS "Found GL version ${OGLPLUS_GL_VERSION_MAJOR}.${OGLPLUS_GL_VERSION_MINOR}")
else()
	message("Could not detect GL version, assuming 3.0")
	set(OGLPLUS_GL_VERSION_MAJOR 3)
	set(OGLPLUS_GL_VERSION_MINOR 0)
endif()
