cmake_policy(SET CMP0076 NEW)

function(add_qwqdsp_attemps ex_file)
    add_executable(qwqdsp-${ex_file}
        ${ex_file}.cpp
    )
    target_link_libraries(qwqdsp-${ex_file} PUBLIC qwqdsp raylib)
    set_target_properties(qwqdsp-${ex_file} PROPERTIES CXX_STANDARD 20)
    set_target_properties(qwqdsp-${ex_file} PROPERTIES FOLDER qwqdsp-example)
endfunction()

add_qwqdsp_attemps(convolution)
add_qwqdsp_attemps(fft_interpolation)
add_qwqdsp_attemps(interpolations)
add_qwqdsp_attemps(minimum_phase_fir)
add_qwqdsp_attemps(freq_shifter)
add_qwqdsp_attemps(fir_design)
add_qwqdsp_attemps(formant_filter)
target_sources(qwqdsp-formant_filter PUBLIC "../playing/slider.cpp")
add_qwqdsp_attemps(dsf)
add_qwqdsp_attemps(blit)
target_sources(qwqdsp-blit PUBLIC "../playing/slider.cpp")