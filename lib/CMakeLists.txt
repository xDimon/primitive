set(DIRS
	cxxopts
)

foreach (D ${DIRS})
	add_subdirectory(${D})
endforeach ()
