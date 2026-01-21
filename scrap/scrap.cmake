
if(GZN_BUILD_EXAMPLES)
	add_subdirectory(${gzn_root}/scrap/examples)
endif()

if(GZN_BUILD_TESTS)
	enable_testing()
	add_subdirectory(${gzn_root}/scrap/tests)
endif()

if(GZN_BUILD_BENCHMARKS)
	enable_testing()
	add_subdirectory(${gzn_root}/scrap/benchmarks)
endif()


