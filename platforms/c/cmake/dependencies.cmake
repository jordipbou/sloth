include(FetchContent)

FetchContent_Declare(
	cpnbi 
  GIT_REPOSITORY https://github.com/jordipbou/CPNBI.git
	GIT_TAG    		 f8d9e468ea4999b472e6c7a682187f4e2ac34176
)

FetchContent_Declare(
	unity
	GIT_REPOSITORY	https://github.com/ThrowTheSwitch/Unity.git
	GIT_TAG					bbf8f3728a937c7627b8094de7ae13559d220ed5	
)

# Fetches all declared deps in one call

FetchContent_MakeAvailable(cpnbi unity)
target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)
if(UNIX)
    target_link_libraries(unity m)
endif()
