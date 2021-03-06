#  Copyright 2010-2013 Matus Chochlik. Distributed under the Boost
#  Software License, Version 1.0. (See accompanying file
#  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

if((wxWidgets_FOUND) AND (OPENAL_FOUND) AND (PNG_FOUND))
	add_definitions(-DOGLPLUS_LINK_LIBRARY=${OGLPLUS_LINK_LIBRARY})
	add_definitions(-DOALPLUS_LINK_LIBRARY=1)

	include_directories(${OPENAL_INCLUDE_DIRS})

	include_directories(${wxWidgets_INCLUDE_DIRS})
	include(${wxWidgets_USE_FILE})

	add_custom_target(oglplus-advanced-example-spectra)

	add_resource_directory(glsl oglplus-advanced-example-spectra)

	add_executable(
		spectra
		EXCLUDE_FROM_ALL
			spectra_app.cpp
			shared_objects.cpp
			main_frame.cpp
			coroutine.cpp
			document_frame.cpp
			document_canvas.cpp
			document_view.cpp
			document.cpp
			openal_document.cpp
			visualisation.cpp
			renderer.cpp
			default_renderer.cpp
			xsection_renderer.cpp
			calculator.cpp
			calculator_cpu.cpp
			calculator_gpu.cpp
	)
	if(${WIN32})
		set_property(TARGET spectra PROPERTY WIN32_EXECUTABLE true)
	endif()

	target_link_libraries(spectra ${wxWidgets_LIBRARIES})
	target_link_libraries(spectra ${OGLPLUS_GL_LIBS})
	target_link_libraries(spectra ${OPENAL_LIBRARIES})
	target_link_libraries(spectra ${PNG_LIBRARIES})
	add_dependencies(oglplus-advanced-example-spectra spectra)
	add_dependencies(oglplus-advanced-examples oglplus-advanced-example-spectra)
endif()
