all: tst

INC=../modules/acm-toolbox/sw

CXXFLAGS=$(addprefix -I,$(INC) .)

tst: tst.cc FTEmul.o
	g++ -o $@ -I. -I ../modules/acm-toolbox/sw/ $^ -L ../modules/acm-toolbox/sw/ -lfwcomm

