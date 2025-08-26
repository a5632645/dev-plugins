function(add_qwqdsp_example ex_file)
    add_executable(qwqdsp-${ex_file}
        ${ex_file}.cpp
    )
    target_link_libraries(qwqdsp-${ex_file} PUBLIC qwqdsp raylib)
    set_target_properties(qwqdsp-${ex_file} PROPERTIES CXX_STANDARD 20)
    set_target_properties(qwqdsp-${ex_file} PROPERTIES FOLDER qwqdsp-example)
endfunction()

add_qwqdsp_example(convolution)
add_qwqdsp_example(fft_interpolation)
add_qwqdsp_example(interpolations)
add_qwqdsp_example(minimum_phase_fir)
add_qwqdsp_example(freq_shifter)
add_qwqdsp_example(fir_design)