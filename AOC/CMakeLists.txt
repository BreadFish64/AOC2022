#find_package(CUDA     REQUIRED)

find_package(fmt      CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_package(Boost    REQUIRED)

add_library(common_pch
    pch.cpp
)
target_link_libraries(common_pch PUBLIC fmt::fmt scn::scn range-v3 Boost::boost std::mdspan)
target_precompile_headers(common_pch PUBLIC pch.hpp)

function(Problem problem_name)

add_executable(${problem_name}
    ${problem_name}.cpp
)

target_link_libraries(${problem_name} PRIVATE common_pch)
target_precompile_headers(${problem_name} REUSE_FROM common_pch)

set_target_properties(${problem_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${problem_name}"
)

add_custom_command(
        TARGET ${problem_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/input/${problem_name}.txt
                ${CMAKE_BINARY_DIR}/${problem_name}/input.txt)

endfunction()

#cuda_add_executable(cuda_test
#    cuda_test.cu
#    OPTIONS --expt-relaxed-constexpr
#)

#target_link_libraries(cuda_test common_pch)

Problem(test)
Problem(calorie_counting)
Problem(rock_paper_scissors)
Problem(rucksack_reorganization)
Problem(camp_cleanup)
Problem(supply_stacks)
Problem(tuning_trouble)
Problem(no_space_left_on_device)
Problem(treetop_tree_house) 
Problem(rope_bridge)
Problem(cathode_ray_tube)
Problem(monkey_in_the_middle)
Problem(hill_climbing_algorithm)
