find_package(fmt           CONFIG REQUIRED)
find_package(range-v3      CONFIG REQUIRED)
find_package(Boost         REQUIRED)
find_package(Eigen3 3.3    REQUIRED NO_MODULE)
find_package(Microsoft.GSL CONFIG REQUIRED)

add_library(common_pch
    pch.cpp
)
target_link_libraries(common_pch PUBLIC fmt::fmt scn::scn range-v3 Boost::boost std::mdspan Eigen3::Eigen Microsoft.GSL::GSL)
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

function(ProblemCU problem_name)

add_executable(${problem_name}
    ${problem_name}.cu
)

target_compile_options(${problem_name} PRIVATE --expt-relaxed-constexpr)

set_target_properties(${problem_name} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

target_link_libraries(${problem_name} PRIVATE common_pch)

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
Problem(distress_signal)
Problem(regolith_reservoir)
Problem(beacon_exclusion_zone)
ProblemCU(beacon_exclusion_zone_cu)
Problem(proboscidea_volcanium)
Problem(pyroclastic_flow)
Problem(boiling_boulders)
Problem(not_enough_minerals)
Problem(grove_positioning_system)
ProblemCU(monkey_math)
Problem(monkey_map)
ProblemCU(unstable_diffusion)
ProblemCU(blizzard_basin)
Problem(full_of_hot_air)
