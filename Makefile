################################
##
################################

CXX := g++
CXX_FLAGS := -Wall -Werror -O2 -fPIC -std=c++11

RM := rm
AR := ar


###
SRC_FILES := mpegts_demux.cpp \
             MpegTsBitStream.cpp \
             MpegTsDemux.cpp

OBJS := ${SRC_FILES:.cpp=.o}

###
EXE := mpegts_demux

### Rules
.PHONY: all
all: mpegts_demux
	@echo "Done!"

.PHONY: mpegts_demux
mpegts_demux: ${OBJS}
	${CXX} $^ -o ${EXE}

.PHONY: clean
clean:
	${RM} -f *.o *~ ${EXE}

%.o: %.cpp
	${CXX} ${CXX_FLAGS} -I. -Iinclude/ -c $<
