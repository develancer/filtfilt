CXX = g++
CXXFLAGS = -Wall -Wextra -O2

.PHONY : clean

filtfilt : filtfilt.cpp

clean :
	@rm -vf filtfilt
