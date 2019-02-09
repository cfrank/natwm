# All external libs

# Add unity
add_library(unity STATIC Unity/src/unity.c)
target_include_directories(unity PUBLIC Unity/src)
