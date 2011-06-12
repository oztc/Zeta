# Add source files here
EXECUTABLE	:= zeta_nvidia
# C/C++ source files (compiled with gcc / c++)
CCFILES		:= zeta.cpp opencl.cpp bitboard.cpp

include ../../common/common_opencl.mk
