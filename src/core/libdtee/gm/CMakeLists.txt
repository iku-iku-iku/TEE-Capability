cmake_minimum_required(VERSION 3.1)
project(MyCryptoProject)

# 设置全局编译选项
include_directories(include)
include_directories(include/miracl)

# 指定 MIRACL 源文件
set(MIRACL_SRC
    source/miracl/mrcore.c
    source/miracl/mrarth0.c
    source/miracl/mrarth1.c
    source/miracl/mrarth2.c
    source/miracl/mrcurve.c
    source/miracl/mrxgcd.c
    source/miracl/mrarth3.c
    source/miracl/mrjack.c
    source/miracl/mrbits.c
    source/miracl/mrmonty.c
    source/miracl/mrsroot.c
    source/miracl/mrlucas.c
    source/miracl/mrrand.c
)

# 创建 MIRACL 静态库
add_library(miracl STATIC ${MIRACL_SRC})

# 指定 SM2 源文件
set(SM2_SRC
    source/SM2_sv.c
    source/SM3.c
    source/SM4.c
    source/SM4_Enc.c
    source/Random.c
)

# 创建 SM2 静态库
add_library(sm2 STATIC ${SM2_SRC})

# 设置目标属性，例如优化级别和警告
target_compile_options(miracl PRIVATE -O2)
target_compile_options(sm2 PRIVATE)

install(TARGETS sm2 DESTINATION /usr/local/lib)
install(TARGETS miracl DESTINATION /usr/local/lib)