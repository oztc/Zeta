DEPTH = ../../../../..

include $(DEPTH)/make/openclsdkdefs.mk 

####
#
#  Targets
#
####

OPENCL			= 1
SAMPLE_EXE		= 1
EXE_TARGET 		= zeta_amd
EXE_TARGET_INSTALL   	= zeta_amd

####
#
#  C/CPP files
#
####

FILES 	= zeta bitboard random opencl
CLFILES	= zeta.cl

include $(DEPTH)/make/openclsdkrules.mk 

