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
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

use work.BasicPkg.all;
use work.CommandMuxPkg.all;

entity CommandWrapperSim is
end entity CommandWrapperSim;

architecture sim of CommandWrapperSim is

   signal clk     : std_logic := '0';
   signal rst     : std_logic_vector(4 downto 0) := (others => '1');

   signal datIbo  : std_logic_vector(7 downto 0);
   signal vldIbo  : std_logic;
   signal rdyIbo  : std_logic;

   signal datObi  : std_logic_vector(7 downto 0);
   signal vldObi  : std_logic;
   signal rdyObi  : std_logic;

   signal abrt    : std_logic;
   signal abrtDon : std_logic;

   signal run     : boolean      := true;

   constant CMD_JTAG_C        : natural := NUM_BASIC_CMDS_C;

   constant CMDS_SUPPORTED_C  : CmdsSupportedType(CMD_JTAG_C to CMD_JTAG_C) := ( others => true );
   constant BUS_L_C           : natural := CMDS_SUPPORTED_C'high;
   constant BUS_R_C           : natural := CMDS_SUPPORTED_C'low;

   constant NUM_CMDS_C        : natural := CMDS_SUPPORTED_C'length;


   signal   bussesIb          : SimpleBusMstArray(BUS_L_C downto BUS_R_C) := (others => SIMPLE_BUS_MST_INIT_C);
   signal   readysIb          : std_logic_vector (BUS_L_C downto BUS_R_C) := (others => '1'                  );
   signal   bussesOb          : SimpleBusMstArray(BUS_L_C downto BUS_R_C) := (others => SIMPLE_BUS_MST_INIT_C);
   signal   readysOb          : std_logic_vector (BUS_L_C downto BUS_R_C) := (others => '1'                  );

   signal   tck, tdi, tms, tdo: std_logic;

begin

   P_CLK : process is
   begin
      if ( run ) then
         wait for 8.33 ns;
         clk <= not clk;
      else
         wait;
      end if;
   end process P_CLK;

   U_DRV : entity work.SimPty
      port map (
         clk          => clk,

         vldOb        => vldIbo,
         datOb        => datIbo,
         rdyOb        => rdyIbo,

         vldIb        => vldObi,
         datIb        => datObi,
         rdyIb        => rdyObi,

         abrt         => abrt,
         abrtDon      => abrtDon
      );

   U_CMD : entity work.CommandWrapper
      generic map (
         SPI_CLK_FREQ_G       => 4.0E5,
         SPI_FREQ_G           => 1.0E5,
         CMDS_SUPPORTED_G     => CMDS_SUPPORTED_C
      )
      port map (
         clk          => clk,
         rst          => rst(rst'left),

         datIb        => datIbo,
         vldIb        => vldIbo,
         rdyIb        => rdyIbo,

         datOb        => datObi,
         vldOb        => vldObi,
         rdyOb        => rdyObi,

         abrt         => abrt,
         abrtDon      => abrtDon,

         boardVersion => x"ff",

         bussesIb     => bussesIb,
         readysIb     => readysIb,
         bussesOb     => bussesOb,
         readysOb     => readysOb
      );

   U_JTAG_BB : entity work.CommandJtagBB
      generic map (
         HPER_DELAY_G => 2
      )
      port map (
         clk          => clk,
         rst          => rst(rst'left),

         mIb          => bussesIb( CMD_JTAG_C ),
         rIb          => readysIb( CMD_JTAG_C ),

         mOb          => bussesOb( CMD_JTAG_C ),
         rOb          => readysOb( CMD_JTAG_C ),

         tck          => tck,
         tms          => tms,
         tdi          => tdi,
         tdo          => tdo
      );

   U_DUT  : entity work.JTAGH19EMUL
      port map (
         clk          => clk,
         rst          => rst(rst'left),

         tck          => tck,
         tms          => tms,

         tdi          => tdi,
         tdo          => tdo,

         jtck         => open,
         jtdi         => open,
         jrstn        => open,
         jshift       => open,
         jupdate      => open,
         jce2         => open,
         ip_enable    => open,
         er2_tdo      => (others => '0')
      );


   P_RST  : process ( clk ) is
   begin
      if ( rising_edge( clk ) ) then
         rst <= rst(rst'left - 1 downto 0) & '0';
      end if;
   end process P_RST;

end architecture sim;
