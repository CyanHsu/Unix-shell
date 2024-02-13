#Makefile for CS6015 Homeworks
#
#Target: msdscript
#clean delets msdscript, and all .o files

HEADERS = *.hpp
CXXSOURCE= *.cpp
CFLAGS = -std=c++11

myUnixShell: $(CXXSOURCE) $(HEADERS)
	$(CXX) $(CFLAGS) -c $(CXXSOURCE)
	$(CXX) $(CFLAGS) -o myUnixShell $(CXXSOURCE) -lreadline

.PHONY: clean
clean: 
	rm -rf *.o myUnixShell *.txt

#.PHONY:
#test: msdscript
#	./msdscript --test
	
.PHONY:
doc:
	cd documentation && doxygen
