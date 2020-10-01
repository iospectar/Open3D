include(ExternalProject)

if (WIN32)
    set(LIBDIR "lib")
else()
    include(GNUInstallDirs)
    set(LIBDIR ${CMAKE_INSTALL_LIBDIR})
endif()

set(FAISS_ROOT "${CMAKE_CURRENT_BINARY_DIR}/libfaiss_install")

ExternalProject_Add(
    ext_faiss
    PREFIX faiss
    GIT_REPOSITORY https://github.com/facebookresearch/faiss.git
    GIT_TAG 27393c436cfd621544a89711fd0694188aa90277
    UPDATE_COMMAND ""
    PATCH_COMMAND git apply ${Open3D_3RDPARTY_DIR}/faiss/0001-Allow-selection-of-blas-library-and-resolve-openmp-error.patch
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${FAISS_ROOT}
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DFAISS_ENABLE_GPU=${BUILD_CUDA_MODULE}
        -DFAISS_ENABLE_PYTHON=OFF
        -DFAISS_BLAS_LIBRARIES=${STATIC_MKL_LIBRARIES}
        -DBUILD_TESTING=OFF
)

if(MSVC)
    set(lib_name "faiss-static")
else()
    set(lib_name "faiss")
endif()

set(FAISS_LIBRARIES ${lib_name})
set(FAISS_INCLUDE_DIR "${FAISS_ROOT}/include/")
set(FAISS_LIB_DIR "${FAISS_ROOT}/lib/")
