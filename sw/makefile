PROGS=tst play
all: $(PROGS)

INC=../modules/acm-toolbox/sw

CXXFLAGS=$(addprefix -I,$(INC) .)

$(PROGS):%: %.cpp FTEmul.o
	g++ -o $@ -I. -I ../modules/acm-toolbox/sw/ $^ -L ../modules/acm-toolbox/sw/ -lfwcomm

clean:
	$(RM) $(PROGS) *.o

