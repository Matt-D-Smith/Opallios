--------------------------------------------------------------------------------
-- Project      : Opallios
--------------------------------------------------------------------------------
-- File         : matrix_64x64.vhd
-- Generated    : 06/13/2022
--------------------------------------------------------------------------------
-- Description  : 64x64 LED matrix with HUB75 interface
--------------------------------------------------------------------------------
-- Revision History :
--   Date        Author        Changes
--   06/13/2022  Matt D Smith  Initial Design
--------------------------------------------------------------------------------
library ieee;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;
library matrix;
    use matrix.matrix_pkg.all;

entity matrix_64x64 IS
    port (
        R0_IN       : in std_logic;
        G0_IN       : in std_logic;
        B0_IN       : in std_logic;
        R1_IN       : in std_logic;
        G1_IN       : in std_logic;
        B1_IN       : in std_logic;
        A_IN        : in std_logic_vector(4 downto 0);
        CLK_IN      : in std_logic;
        BLANK_IN    : in std_logic;
        LATCH_IN    : in std_logic;

        R0_OUT      : out std_logic;
        G0_OUT      : out std_logic;
        B0_OUT      : out std_logic;
        R1_OUT      : out std_logic;
        G1_OUT      : out std_logic;
        B1_OUT      : out std_logic;
        A_OUT       : out std_logic_vector(4 downto 0);
        CLK_OUT     : out std_logic;
        BLANK_OUT   : out std_logic;
        LATCH_OUT   : out std_logic;
        MATRIX_TB   : out t_RGB_matrix
    );
end matrix_64x64;

architecture arch of matrix_64x64 is

    component ICN2037 IS
    port (
        CLK         : in std_logic;
        DIN         : in std_logic;
        LE          : in std_logic;
        OE_n        : in std_logic;
        DOUT        : out std_logic;
        CCOUT_n     : out std_logic_vector(15 DOWNTO 0)
    );
    end component ICN2037;

    signal row_en       : std_logic_vector(31 downto 0);
    signal RGBRGB_in    : std_logic_vector(5 downto 0);
    signal RGBRGB_1     : std_logic_vector(5 downto 0);
    signal RGBRGB_2     : std_logic_vector(5 downto 0);
    signal RGBRGB_3     : std_logic_vector(5 downto 0);
    signal RGBRGB_out  : std_logic_vector(5 downto 0);

    type t_col is array (5 downto 0) of std_logic_vector(63 downto 0);
    signal COL_RGBRGB   : t_col;

begin

    RGBRGB_in <= B1_IN & G1_IN & R1_IN & B0_IN & G0_IN & R0_IN;

    g_cols : for i in 0 to 5 generate

        u_ICN2037_1 : ICN2037
        port map (
            CLK         => CLK_IN,
            DIN         => RGBRGB_in(i),
            LE          => LATCH_IN,
            OE_n        => BLANK_IN,
            DOUT        => RGBRGB_1(i),
            CCOUT_n     => COL_RGBRGB(i)(15 downto 0)
        );

        u_ICN2037_2 : ICN2037
        port map (
            CLK         => CLK_IN,
            DIN         => RGBRGB_1(i),
            LE          => LATCH_IN,
            OE_n        => BLANK_IN,
            DOUT        => RGBRGB_2(i),
            CCOUT_n     => COL_RGBRGB(i)(31 downto 16)
        );

        u_ICN2037_3 : ICN2037
        port map (
            CLK         => CLK_IN,
            DIN         => RGBRGB_2(i),
            LE          => LATCH_IN,
            OE_n        => BLANK_IN,
            DOUT        => RGBRGB_3(i),
            CCOUT_n     => COL_RGBRGB(i)(47 downto 32)
        );

        u_ICN2037_4 : ICN2037
        port map (
            CLK         => CLK_IN,
            DIN         => RGBRGB_3(i),
            LE          => LATCH_IN,
            OE_n        => BLANK_IN,
            DOUT        => RGBRGB_out(i),
            CCOUT_n     => COL_RGBRGB(i)(63 downto 48)
        );

    end generate; -- g_cols

    R0_OUT <= RGBRGB_out(0);
    G0_OUT <= RGBRGB_out(1);
    B0_OUT <= RGBRGB_out(2);
    R1_OUT <= RGBRGB_out(3);
    G1_OUT <= RGBRGB_out(4);
    B1_OUT <= RGBRGB_out(5);

    LATCH_OUT <= LATCH_IN;
    BLANK_OUT <= BLANK_IN;
    CLK_OUT   <= CLK_IN;
    A_OUT     <= A_IN;

    matrix_tb_proc : process( COL_RGBRGB, A_IN)
    begin
        MATRIX_TB <= (others => (others => (others => '0')));
        MATRIX_TB(0)(to_integer(unsigned(A_IN))) <= not COL_RGBRGB(0);
        MATRIX_TB(1)(to_integer(unsigned(A_IN))) <= not COL_RGBRGB(1);
        MATRIX_TB(2)(to_integer(unsigned(A_IN))) <= not COL_RGBRGB(2);
        MATRIX_TB(0)(to_integer(unsigned(A_IN))+32) <= not COL_RGBRGB(3);
        MATRIX_TB(1)(to_integer(unsigned(A_IN))+32) <= not COL_RGBRGB(4);
        MATRIX_TB(2)(to_integer(unsigned(A_IN))+32) <= not COL_RGBRGB(5);
    end process; -- matrix_tb_proc

end arch;