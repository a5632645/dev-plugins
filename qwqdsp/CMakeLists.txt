add_library(qwqdsp
    STATIC
        "./source/oouras.cpp"
)
target_include_directories(qwqdsp PUBLIC "./include")
set_target_properties(qwqdsp PROPERTIES CXX_STANDARD 20)
