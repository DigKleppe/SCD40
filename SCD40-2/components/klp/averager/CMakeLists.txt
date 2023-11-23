list(APPEND srcs "averager.cpp")

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "include")

