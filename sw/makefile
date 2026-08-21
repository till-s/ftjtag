PYTHONDIR=/usr/include/python3.12
FTDIR=../../ftd2xx/linux-x86_64
PROGS=tst play ana
all: $(PROGS)

MODS=../modules/acm-toolbox/sw ../modules/jtag-utils/sw
VPATH=$(MODS)

CXXFLAGS+=$(addprefix -I,$(MODS) .)

%.o: %.cpp
	g++ -o $@ $(CXXFLAGS) -c $<

libjta.a: FTEmul.o JtagTap.o JtagAna.o
	ar cr $@ $^

$(PROGS):%: %.cpp libjta.a
	g++ -o $@ $(CXXFLAGS) $^ -L ../modules/acm-toolbox/sw/ -L. -ljta -lfwcomm

clean:
	$(RM) $(PROGS) *.o

%.cpp: %.pyx
	cython3 --cplus $^

pyMPSSE.so: pyMPSSE.cpp MPSSE.cpp MPSSE.hpp
	g++ -shared -fPIC -o $@ $(addprefix -I, $(PYTHONDIR) . $(FTDIR)) $(filter %.cpp, $^) $(addprefix -L, $(FTDIR)) -lftd2xx -Wl,-rpath,$(realpath $(FTDIR))

pyFTEmul.so: pyFTEmul.cpp FTEmul.cpp FTEmul.hpp
	g++ -shared -fPIC -o $@ $(CXXFLAGS) $(addprefix -I, $(PYTHONDIR) . $(FTDIR)) $(filter %.cpp, $^) $(addprefix -L,$(MODS)) -lfwcomm
