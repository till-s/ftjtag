
VPATH+=../modules/jtag-utils/hdl
VPATH+=../modules/acm-toolbox/hdl

GHDL=ghdl

CommandWrapperSim: CommandWrapperSim.o SimPtyIO.o

CommandWrapperSim:
	$(GHDL) -e $(GHDLFLAGS) -Wl,SimPtyIO.o -o $@  $@

CommandWrapperSim.o: CommandJtagBB.o BasicPkg.o JTAGH19EMUL.o CommandWrapper.o

CommandWrapper.o: GenRegPkg.o ByteDeStuffer.o ByteStuffer.o SimpleBusPipeStage.o CommandMux.o CommandSpi.o CommandGenRegs.o CommandVersion.o SimPty.o
GenRegPkg.o: RegPkg.o

CommandGenRegs.o: CommandReg.o
CommandReg.o: SimpleBusAsync.o
SimpleBusAsync.o: SynchronizerBit.o

CommandMux.o: CommandMuxPkg.o ILAWrapperPkg.o
CommandSpi.o: SpiBitShifter.o

JTAGH19EMUL.o: JtagTap.o JtagTapPkg.o
JtagTap.o: JtagTapPkg.o


CommandJtagBB.o: CommandMuxPkg.o

%.o: %.vhd
	$(GHDL) -a $<

clean:
	$(RM) *.o CommandWrapperSim work-*.cf

.PHONY: clean
