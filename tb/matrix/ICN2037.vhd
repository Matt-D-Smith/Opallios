--------------------------------------------------------------------------------
-- Project      : Opallios
--------------------------------------------------------------------------------
-- File         : ICN2037.vhd
-- Generated    : 06/13/2022
--------------------------------------------------------------------------------
-- Description  : 16 channel constant current LED sink driver with dual latch
--------------------------------------------------------------------------------
-- Revision History :
--   Date        Author        Changes
--   06/13/2022  Matt D Smith  Initial Design
--------------------------------------------------------------------------------
library ieee ;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;

entity ICN2037 IS
    port (
        CLK         : in std_logic;
        DIN         : in std_logic;
        LE          : in std_logic;
        OE_n        : in std_logic;
        DOUT        : out std_logic;
        CCOUT_n     : out std_logic_vector(15 DOWNTO 0)
    );
end ICN2037;

architecture arch of ICN2037 is

    signal sreg : std_logic_vector(15 downto 0);
    signal latch : std_logic_vector(15 downto 0);
    signal reg2 : std_logic_vector(15 downto 0);

begin

    din_proc : process (CLK)
    begin
        if rising_edge(CLK) then
            sreg <= sreg(14 downto 0) & DIN;
            DOUT <= sreg(15);
        end if;
    end process din_proc;

    latch_proc : process (LE, sreg, latch)
    begin
        latch <= latch;
        if LE = '1' then
            latch <= sreg;
        end if;
    end process latch_proc;

    OE_proc : process (OE_n)
    begin
        reg2 <= reg2;
        if falling_edge(OE_n) then
            reg2 <= latch;
        end if;
    end process OE_proc;

    CCOUT_n <= NOT reg2 when OE_n = '0' else (others => '1') ;

end arch;