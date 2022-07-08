--------------------------------------------------------------------------------
-- Project      : Opallios
--------------------------------------------------------------------------------
-- File         : matrix_interface.vhd
-- Generated    : 07/06/2022
--------------------------------------------------------------------------------
-- Description  : 64x64 LED matrix interface module with HUB75 interface
--------------------------------------------------------------------------------
library ieee;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;

entity matrix_interface IS
    port (
        CLK             : in  std_logic;
        LED_Addr        : out  std_logic_vector(11 downto 0); -- log2(64*64)
        LED_Data_RGB    : in  std_logic_vector(17 downto 0); -- 18 bit color
        R0              : out std_logic;
        G0              : out std_logic;
        B0              : out std_logic;
        R1              : out std_logic;
        G1              : out std_logic;
        B1              : out std_logic;
        Matrix_Addr     : out std_logic_vector(4 downto 0);
        Matrix_CLK      : out std_logic;
        BLANK           : out std_logic;
        LATCH           : out std_logic
    );
end matrix_interface;

architecture rtl of matrix_interface is

begin

    -- pretend there is an architecture here
        R0              <= '1';
        G0              <= '1';
        B0              <= '1';
        R1              <= '1';
        G1              <= '1';
        B1              <= '1';
        LED_Addr        <= (others => '1') ;
        Matrix_CLK      <= '1';
        BLANK           <= '1';
        LATCH           <= '1';

end architecture;