set(TARGET lambo4j)

if(APPLE)
    execute_process(
            COMMAND /usr/libexec/java_home
            OUTPUT_VARIABLE JAVA_HOME
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(JAVA_INCLUDE_PATH "${JAVA_HOME}/include")
    set(JAVA_INCLUDE_PATH2 "${JAVA_HOME}/include/darwin")
else()
    set(JAVA_HOME $ENV{JAVA_HOME})
    set(JAVA_INCLUDE_PATH "${JAVA_HOME}/include")
    set(JAVA_INCLUDE_PATH2 "${JAVA_HOME}/include/win32")
    # On Windows you get a Java UnsatisfiedLinkError without this flag
    IF(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MDb")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
endif()

include_directories(
        ${JAVA_INCLUDE_PATH}
        ${JAVA_INCLUDE_PATH2}
)

add_library(${TARGET} SHARED
        llamawrapper.cpp
        jni_implementation.cpp
)

install(TARGETS ${TARGET} LIBRARY)

target_link_libraries(${TARGET} PRIVATE
        llama
        ${CMAKE_THREAD_LIBS_INIT}
)

target_compile_features(${TARGET} PRIVATE cxx_std_17)