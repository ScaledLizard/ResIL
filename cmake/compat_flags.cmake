include(TestCXXAcceptsFlag)

# try to use compiler flag -std=c++11
MACRO(AddSTDFlag FLAG)
	set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${FLAG} ")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG} ")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${FLAG} ")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} ${FLAG} ")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${FLAG} ")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${FLAG} ")
	set(CXX_STD0X_FLAGS "${FLAG}" )
ENDMACRO(AddSTDFlag FLAG)


set(CXX_STD0X_FLAGS FALSE)
set(i 0)
foreach(f -std=gnu++11 -std=c++11 -std=c++0x)
        MATH(EXPR i "${i}+1") #cmake has working unset :-|
        CHECK_CXX_ACCEPTS_FLAG("${f}" ACCEPTSFLAG${i})
        if(${ACCEPTSFLAG${i}} AND NOT ${CXX_STD0X_FLAGS})
                message(STATUS "Using ${f}")
                AddSTDFlag("${f}")
                set(CXX_STD0X_FLAGS TRUE)
        endif()
endforeach()

if(NOT CXX_STD0X_FLAGS)
        message(FATAL_ERROR "you need a c++11 compatible compiler")
endif()

