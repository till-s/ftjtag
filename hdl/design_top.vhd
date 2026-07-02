--LB-MIT
--
-- MIT License
--
-- Copyright (c) 2026 Till Straumann
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
-- SOFTWARE.
--
--LE-MIT

library ieee;
use     ieee.std_logic_1164.all;
use     ieee.numeric_std.all;

use     work.UlpiPkg.all;
use     work.Usb2Pkg.all;
use     work.Usb2UtilPkg.all;
use     work.Usb2AppCfgPkg.all;
use     work.Usb2DescPkg.all;
use     work.Usb2MuxEpCtlPkg.all;
use     work.Usb2EpGenericCtlPkg.all;
use     work.Usb2AppCfgPkg.all;
use     work.CommandMuxPkg.all;
use     work.BasicPkg.Slv8Array;
use     work.GitVersionPkg.all;
use     work.RegPkg.all;
use     work.GenRegPkg.all;

-- differences to V1
--
--       V2.1         V1         PIN      Comments (using design on V1 board)
--   fpgaGpio(5)    eth_txd(3)    54      dont' use
--   fpgaGpio(6)    eth_txd(2)    55      dont' use
--   fpgaGpio(7)    eth_rxd(2)    65      dont' use
--   gpsRstb        eth_rxd(3)    66      open or pull-up
--   gpsPps         fpgaGpio(7)   45      [3]
--   gpsRx          fpgaGpio(5)   42      UART RX on fpgaGpio(5) (fpga OUT)
--   gpsTx          fpgaGpio(6)   43      UART TX on fpgaGpio(6) (fpga INP)
--   N/C [1]        fpgaGpio(0)   36      don't use
--   fpgaGpio(0)    N/C [2]       32      don't use
--   eth_gpio_1     fpga_b3_io_2  89      don't use
--   eth_gpio_2     fpga_b3_io_1  90      don't use
--   eth_clk_out    fpga_b3_io_0  93      don't use
--
-- NOTES:
--   [1] connected to fpgaGpio(0) on V2.0
--   [2] N/C on V2.0
--   [3] pulldown on this pin not supported; if spurious LED signal
--       is a problem then strap fpgaGpio(7) to GND (V1 board).
--
entity design_top is
   port (
      ulpiClk           : in    std_logic;
      -- NOTE    : unfortunately, the ulpiClk stops while ulpiRstb is asserted...
      ulpiRstb          : out   std_logic                    := '1';
      ulpiDat_IN        : in    std_logic_vector(7 downto 0) := (others => '0');
      ulpiDat_OUT       : out   std_logic_vector(7 downto 0) := (others => '0');
      ulpiDat_OE        : out   std_logic_vector(7 downto 0) := (others => '0');
      ulpiDir           : in    std_logic                    := '0';
      ulpiNxt           : in    std_logic                    := '0';
      ulpiStp_IN        : in    std_logic                    := '0';
      ulpiStp_OUT       : out   std_logic                    := '0';
      ulpiStp_OE        : out   std_logic                    := '0';
      LED               : out   std_logic_vector(7 downto 0) := (others => '1');
      ulpiPllLocked     : in    std_logic;

      -- reconfiguration
      cfg_CONFIG        : out   std_logic                    := '0';
      cfg_ENA           : out   std_logic                    := '0';
      cfg_CBSEL         : out   std_logic_vector(1 downto 0) := (others => '0');
      cfg_CDONE         : in    std_logic;
      cfg_ERROR         : in    std_logic;

      spiSClk           : out   std_logic;
      spiMOSI           : out   std_logic;
      spiMISO           : in    std_logic;
      spiCSb_OUT        : out   std_logic;
      spiCSb_OE         : out   std_logic := '1';
      spiCSb_IN         : in    std_logic;

      -- note: fpgaGpio[0] *not* available on V2.0 board (only starting with 2.1)
      fpgaGpio_IN       : in    std_logic_vector(7 downto 1) := (others => '0');
      fpgaGpio_OUT      : out   std_logic_vector(7 downto 1) := (others => '0');
      fpgaGpio_OE       : out   std_logic_vector(7 downto 1) := (others => '0');

      fpga_b3_io_IN     : in    std_logic_vector(2 downto 0);
      fpga_b3_io_OUT    : out   std_logic_vector(2 downto 0) := (others => '0');
      fpga_b3_io_OE     : out   std_logic_vector(2 downto 0) := (others => '0')
   );
end entity design_top;

architecture rtl of design_top is

   attribute ASYNC_REG         : string;
   attribute SYN_PRESERVE      : boolean;

   constant BOARD_VERSION_C    : std_logic_vector(7 downto 0) := x"10";

   -- must cover bulk max pkt size
   constant LD_FIFO_OUT_C      : natural :=  9;
   constant LD_FIFO_INP_C      : natural :=  9;

   -- AUD_SMPL_FREQ_C should divide the ULPI clock (60MHz)
   constant ULPI_CLK_FREQ_C    : natural := 60000000;
   constant JTAG_CLK_FREQ_C    : natural := 6000000;
   constant JTAG_HPER_C        : natural := ULPI_CLK_FREQ_C/JTAG_CLK_FREQ_C/2 - 1;

   constant CMD_JTAG_C         : natural := NUM_BASIC_CMDS_C;

   constant CMDS_SUPPORTED_C   : CmdsSupportedType(CMD_JTAG_C to CMD_JTAG_C) := ( others => true );
   constant BUS_L_C            : natural := CMDS_SUPPORTED_C'high;
   constant BUS_R_C            : natural := CMDS_SUPPORTED_C'low;

   constant NUM_CMDS_C         : natural := CMDS_SUPPORTED_C'length;


   signal acmFifoOutDat        : Usb2ByteType;
   signal acmFifoOutEmpty      : std_logic;
   signal acmFifoOutRen        : std_logic    := '1';
   signal acmFifoOutVld        : std_logic    := '0';
   signal acmFifoInpDat        : Usb2ByteType := (others => '0');
   signal acmFifoInpFull       : std_logic;
   signal acmFifoInpWen        : std_logic    := '0';

   signal acmFifoInpMinFill    : unsigned(LD_FIFO_INP_C - 1 downto 0) := (others=> '0');
   signal acmFifoInpTimer      : unsigned(32 - 1 downto 0) := (others=> '0');

   signal acmFifoLocal         : std_logic    := '1';

   signal acmDTR               : std_logic;
   signal acmSelUart           : std_logic;
   signal acmRTS               : std_logic;
   signal acmRate              : unsigned(31 downto 0);
   signal acmStopBits          : unsigned( 1 downto 0);
   signal acmDataBits          : unsigned( 4 downto 0);
   signal acmParity            : unsigned( 2 downto 0);

   signal acmLineBreak         : std_logic := '0';
   signal acmOverRun           : std_logic := '0';
   signal acmParityError       : std_logic := '0';
   signal acmFramingError      : std_logic := '0';
   signal acmRingDetect        : std_logic := '0';
   signal acmBreakState        : std_logic := '0';

   signal acmFifoRst           : std_logic    := '0';

   signal usb2Rst              : std_logic := '0';
   signal usb2DevStatus        : Usb2DevStatusType := USB2_DEV_STATUS_INIT_C;

   signal ulpiIb               : UlpiIbType := ULPI_IB_INIT_C;
   signal ulpiOb               : UlpiObType := ULPI_OB_INIT_C;
   signal ulpiRx               : UlpiRxType;
   signal ulpiRst              : std_logic := '0';
   signal ulpiForceStp         : std_logic := '0';
   signal usb2HiSpeedEn        : std_logic := '1';
   signal ulpiDirB             : std_logic;
   signal ulpiClkBlink         : std_logic;

   signal fifoWRdy             : std_logic;

   signal ledDiagRegs          : Usb2ByteArray(0 to 1) := (others => (others => '0'));
   signal ledIn                : std_logic_vector(LED'range);

   signal usb2DisconnectReq    : std_logic;
   signal usb2DisconnectAck    : std_logic;

   signal genRegReq            : GenRegOutType;
   signal genRegRep            : GenRegInpType;

   signal bussesIb             : SimpleBusMstArray(BUS_L_C downto BUS_R_C) := (others => SIMPLE_BUS_MST_INIT_C);
   signal readysIb             : std_logic_vector (BUS_L_C downto BUS_R_C) := (others => '1'                  );
   signal bussesOb             : SimpleBusMstArray(BUS_L_C downto BUS_R_C) := (others => SIMPLE_BUS_MST_INIT_C);
   signal readysOb             : std_logic_vector (BUS_L_C downto BUS_R_C) := (others => '1'                  );

   signal tck                  : std_logic;
   signal tms                  : std_logic;
   signal tdi                  : std_logic;
   signal tdo                  : std_logic;

begin

   P_INI : process ( ulpiClk ) is
      variable cnt : unsigned(29 downto 0)        := (others => '1');
      variable rst : std_logic_vector(3 downto 0) := (others => '1');
      attribute ASYNC_REG of rst : variable is "TRUE";
   begin
      if ( rising_edge( ulpiClk ) ) then
         if ( cnt( cnt'left ) = '1' ) then
            cnt := cnt - 1;
         end if;
         rst := not ulpiPllLocked & rst(rst'left downto 1);
      end if;
      ulpiRst      <= rst(0);
      usb2Rst      <= rst(0);
   end process P_INI;

   acmFifoOutVld <= not acmFifoOutEmpty;
   fifoWRdy      <= not acmFifoInpFull;

   U_CMD : entity work.CommandWrapper
   generic map (
      GIT_VERSION_G                => GIT_VERSION_C,
      SPI_CLK_FREQ_G               => real(ULPI_CLK_FREQ_C),
      CMDS_SUPPORTED_G             => CMDS_SUPPORTED_C
   )
   port map (
      clk                          => ulpiClk,
      rst                          => acmFifoRst,

      boardVersion                 => BOARD_VERSION_C,

      datIb                        => acmFifoOutDat,
      vldIb                        => acmFifoOutVld,
      rdyIb                        => acmFifoOutRen,
      datOb                        => acmFifoInpDat,
      vldOb                        => acmFifoInpWen,
      rdyOb                        => fifoWRdy,

      genRegOb                     => genRegReq,
      genRegIb                     => genRegRep,

      spiSClk                      => spiSClk,
      spiMOSI                      => spiMOSI,
      spiCSb                       => spiCSb_OUT,
      spiMISO                      => spiMISO,

      bussesIb                     => bussesIb,
      readysIb                     => readysIb,
      bussesOb                     => bussesOb,
      readysOb                     => readysOb
   );

   U_JTAG : entity work.CommandJtagBB
   generic map (
      HPER_DELAY_G                 => JTAG_HPER_C
   )
   port map (
      clk                          => ulpiClk,
      rst                          => usb2Rst,

      mIb                          => bussesIb( CMD_JTAG_C ),
      rIb                          => readysIb( CMD_JTAG_C ),

      mOb                          => bussesOb( CMD_JTAG_C ),
      rOb                          => readysOb( CMD_JTAG_C ),

      tck                          => tck,
      tms                          => tms,
      tdi                          => tdi,
      tdo                          => tdo
   );

   ulpiDat_OUT   <= ulpiOb.dat;
   ulpiIb.dat    <= ulpiDat_IN;
   ulpiDat_OE    <= (others => ulpiDirB);

   ulpiDirB      <= not ulpiDir;
   ulpiIb.dir    <= ulpiDir;

   ulpiStp_OUT   <= ulpiOb.stp;
   ulpiIb.stp    <= ulpiStp_IN;
   ulpiStp_OE    <= '1';

   ulpiIb.nxt    <= ulpiNxt;

   process ( ulpiClk ) is
      variable cnt : unsigned(25 downto 0) := (others => '0');
   begin
      if ( rising_edge( ulpiClk ) ) then
         cnt := cnt + 1;
      end if;
      ulpiClkBlink <= cnt(cnt'left);
   end process;

   U_USB_DEV : entity work.Usb2ExampleDev
      generic map (
         ULPI_CLK_MODE_INP_G       => false,
         DESCRIPTORS_G             => USB2_APP_DESCRIPTORS_C,
         DESCRIPTORS_BRAM_G        => true,
         LD_ACM_FIFO_DEPTH_INP_G   => LD_FIFO_INP_C,
         LD_ACM_FIFO_DEPTH_OUT_G   => LD_FIFO_OUT_C,
         CDC_ACM_ASYNC_G           => false,
         ULPI_EMU_MODE_G           => NONE,
         MARK_DEBUG_ULPI_IO_G      => false,
         MARK_DEBUG_PKT_TX_G       => false,
         MARK_DEBUG_PKT_RX_G       => false,
         MARK_DEBUG_PKT_PROC_G     => false
      )
      port map (
         usb2Clk                   => ulpiClk,
         usb2Rst                   => usb2Rst,
         usb2RstOut                => open,
         ulpiRst                   => ulpiRst,
         ulpiIb                    => ulpiIb,
         ulpiOb                    => ulpiOb,
         ulpiRx                    => ulpiRx,
         ulpiForceStp              => ulpiForceStp,

         usb2HiSpeedEn             => usb2HiSpeedEn,

         usb2DevStatus             => usb2DevStatus,
         usb2DisconnectReq         => usb2DisconnectReq,
         usb2DisconnectAck         => usb2DisconnectAck,

         acmFifoClk                => ulpiClk,
         acmFifoOutDat             => acmFifoOutDat,
         acmFifoOutEmpty           => acmFifoOutEmpty,
         acmFifoOutRen             => acmFifoOutRen,
         acmFifoInpDat             => acmFifoInpDat,
         acmFifoInpFull            => acmFifoInpFull,
         acmFifoInpWen             => acmFifoInpWen,

         acmFifoInpMinFill         => acmFifoInpMinFill,
         acmFifoInpTimer           => acmFifoInpTimer,
         acmFifoLocal              => acmFifoLocal,

         acmDTR                    => acmDTR,
         acmRTS                    => acmRTS,

         acmRate                   => acmRate,
         acmStopBits               => acmStopBits,
         acmParity                 => acmParity,
         acmDataBits               => acmDataBits,

         acmLineBreak              => acmLineBreak,
         acmOverRun                => acmOverRun,
         acmParityError            => acmParityError,
         acmFramingError           => acmFramingError,
         acmRingDetect             => acmRingDetect,
         acmBreakState             => acmBreakState
      );

   -- mux the MDIO control endpoint between interface (driver use) and device (user/diagnostic)

   ledIn(7)                  <= '0';
   ledIn(6)                  <= '0';
   ledIn(5)                  <= usb2DevStatus.suspended;
   ledIn(4)                  <= '0';
   ledIn(3)                  <= '0';
   ledIn(2)                  <= '0';
   ledIn(1)                  <= '0';
   ledIn(0)                  <= '0'; --gpsPps;

   fpgaGpio_OUT(1)           <= tck;
   fpgaGpio_OE (1)           <= not genRegReq.scratch(0);
   fpgaGpio_OUT(2)           <= tms;
   fpgaGpio_OE (2)           <= not genRegReq.scratch(0);
   fpgaGpio_OUT(3)           <= tdi;
   fpgaGpio_OE (3)           <= not genRegReq.scratch(0);
   fpgaGpio_OE (4)           <= '0';
   tdo                       <= fpgaGpio_IN(4);

   -- LEDs are active low
   LED                       <= not ((ledIn and not ledDiagRegs(1)) or (ledDiagRegs(1) and ledDiagRegs(0))) ;

   -- reconfiguration interface
   genRegRep.reconfigurable  <= '1';
   cfg_ENA                   <= genRegRep.reconfigurable;
   -- initiate device disconnect from usb when reconfiguration is
   -- requested as soon as DTR drops.
   usb2DisconnectReq         <= (genRegReq.reconfigure and not acmDTR);
   -- once disconnection is complete proceed to reconfigure
   cfg_CONFIG                <= usb2DisconnectAck;

end architecture rtl;
