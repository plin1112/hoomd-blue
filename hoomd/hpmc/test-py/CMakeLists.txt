enable_testing()

macro(add_script_test_cpu_mpi script)
    # execute on two processors
    SET(nproc 2)
    if (ENABLE_MPI)
        if(NOT "${EXCLUDE_FROM_MPI}" MATCHES ${script})
            add_test(NAME ${script}-mpi-cpu
                COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${nproc}
                ${MPIEXEC_POSTFLAGS} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${script} "--mode=cpu")
                set_tests_properties(${script}-mpi-cpu PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}")
            endif()
    endif(ENABLE_MPI)
endmacro()

macro(add_script_test_gpu_mpi script)
    # execute on two processors
    SET(nproc 2)
    if (ENABLE_MPI)
        if(NOT "${EXCLUDE_FROM_MPI}" MATCHES ${script})
            add_test(NAME ${script}-mpi-gpu
                COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${nproc}
                ${MPIEXEC_POSTFLAGS} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${script} "--mode=gpu" "--gpu_error_checking")
            set_tests_properties(${script}-mpi-gpu PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}")
        endif()
    endif(ENABLE_MPI)
endmacro()


macro(add_script_test_cpu script)
    add_test(${script}-cpu ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${script} "--mode=cpu")
    set_tests_properties(${script}-cpu PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}")

    add_script_test_cpu_mpi(${script})
endmacro()

macro(add_script_test_gpu script)
    add_test(${script}-gpu ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${script} "--mode=gpu")
    set_tests_properties(${script}-gpu PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}")

    add_script_test_gpu_mpi(${script})
endmacro()

set(TEST_LIST_CPU
    npt.py
    small-box-2d.py
    small-box-3d.py
    stats_check.py
    test_util.py
    ignore_flag.py
    move_by_type.py
    shape_union.py
    create_shapes.py
    image-list.py
    test_sdf.py
    test_implicit.py
    test_ghost_layer.py
    test_walls.py
    max_verts.py
    muvt.py
    meta_data.py
    )

set(TEST_LIST_GPU
    shape_union.py
    stats_check.py
    ignore_flag.py
    move_by_type.py
    test_sdf.py
    test_implicit.py
    test_ghost_layer.py
    max_verts.py
    muvt.py
    )

set(EXCLUDE_FROM_MPI
    small-box-2d.py
    small-box-3d.py
    test_util.py
    ignore_flag.py
    npt.py
    image-list.py
    hoomd.util.py
    max_verts.py
    create_shapes.py
   )

set(MPI_ONLY
    gibbs.py
    )

foreach (CUR_TEST ${TEST_LIST_CPU})
    add_script_test_cpu(${CUR_TEST})
endforeach (CUR_TEST)

foreach (CUR_TEST ${MPI_ONLY})
    add_script_test_cpu_mpi(${CUR_TEST})
endforeach (CUR_TEST)

if (ENABLE_CUDA)
foreach (CUR_TEST ${TEST_LIST_GPU})
    add_script_test_gpu(${CUR_TEST})
endforeach (CUR_TEST)
foreach (CUR_TEST ${MPI_ONLY})
    add_script_test_gpu_mpi(${CUR_TEST})
endforeach (CUR_TEST)
endif (ENABLE_CUDA)