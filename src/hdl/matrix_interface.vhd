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
        RSTn            : in  std_logic;
        LED_Data_RGB_lo : in  std_logic_vector(17 downto 0); -- 18 bit color
        LED_Data_RGB_hi : in  std_logic_vector(17 downto 0); -- 18 bit color
        LED_RAM_Addr    : out std_logic_vector(10 downto 0); -- log2(64*64)
        R0              : out std_logic;
        G0              : out std_logic;
        B0              : out std_logic;
        R1              : out std_logic;
        G1              : out std_logic;
        B1              : out std_logic;
        Matrix_Addr     : out std_logic_vector(4 downto 0);
        Matrix_CLK      : out std_logic;
        BLANK           : out std_logic;
        LATCH           : out std_logic;
        Next_Frame      : out std_logic;
        TP              : out std_logic_vector(7 downto 0)
    );
end matrix_interface;

architecture rtl of matrix_interface is

    component matrix_control_sm is
        port (
            CLK             : in  std_logic;
            RSTn            : in  std_logic;
            CLK_Matrix      : in  std_logic;
            LED_RAM_Addr    : out std_logic_vector(10 downto 0); -- log2(64*64/2)
            LED_RAM_Load    : out std_logic;
            Next_Frame      : out std_logic;
            Shift_Data      : out std_logic;
            Shift_Data_Cntr : out std_logic_vector(5 downto 0);
            Matrix_CLK_Gate : out std_logic;
            Blank           : out std_logic;
            Latch           : out std_logic;
            RGB_bit_count   : out std_logic_vector(2 downto 0);
            TP              : out std_logic_vector(7 downto 0)
        );
    end component;

    signal CLK_matrix       : std_logic;
    signal Matrix_CLK_Gate  : std_logic;
    signal Clk_Div_Count    : unsigned(3 downto 0) := (others => '0'); -- 12.5 mhz for debugging, restore to 25mhz for actual -- 25 MHz
    signal LED_RAM_Load     : std_logic;
    signal LED_RAM_Load_d   : std_logic;
    signal LED_RAM_Load_q   : std_logic;
    signal RGB_bit_count    : std_logic_vector(2 downto 0) := (others => '0');
    signal Shift_Data       : std_logic;
    signal Shift_Data_Cntr  : std_logic_vector(5 downto 0) := (others => '0');
    signal Matrix_Addr_d    : std_logic_vector(Matrix_Addr'length-1 downto 0);
    signal Matrix_Addr_q    : std_logic_vector(Matrix_Addr'length-1 downto 0);
    signal LED_RAM_Addr_int : std_logic_vector(LED_RAM_Addr'length-1 downto 0);
    signal Latch_int        : std_logic;
    

    -- type Row_Data_t is array (63 downto 0) of std_logic_vector(2 downto 0);
    -- signal Row_Data_lo : Row_Data_t;
    -- signal Row_Data_hi : Row_Data_t;
    signal R0_Data : std_logic_vector(63 downto 0);
    signal G0_Data : std_logic_vector(63 downto 0);
    signal B0_Data : std_logic_vector(63 downto 0);
    signal R1_Data : std_logic_vector(63 downto 0);
    signal G1_Data : std_logic_vector(63 downto 0);
    signal B1_Data : std_logic_vector(63 downto 0);

begin

    p_clk_div : process (CLK)
    begin
        if rising_edge(CLK) then
            Clk_Div_Count <= Clk_Div_Count + 1;
        end if;
    end process;
    CLK_Matrix <= Clk_Div_Count(3); -- 12.5 mhz for debugging, restore to 25mhz for actual -- 25 MHz internal use
    Matrix_CLK <= CLK_Matrix when (Matrix_CLK_Gate = '1') else '0'; -- output

    u_matrix_sm : matrix_control_sm
    port map (
        CLK             => CLK,
        RSTn            => RSTn,
        CLK_Matrix      => CLK_Matrix,
        LED_RAM_Addr    => LED_RAM_Addr_int,
        LED_RAM_Load    => LED_RAM_Load,
        Next_Frame      => Next_Frame,
        Shift_Data      => Shift_Data,
        Shift_Data_Cntr => Shift_Data_Cntr,
        Matrix_CLK_Gate => Matrix_CLK_Gate,
        Blank           => BLANK,
        Latch           => LATCH_int,
        RGB_bit_count   => RGB_bit_count,
        TP              => TP
    );

    p_load_data : process (CLK)
    begin
        if rising_edge(CLK) then
            Matrix_Addr_d <= Matrix_Addr_q;
            Matrix_Addr_q <= Matrix_Addr_d;
            LED_RAM_Load_d <= LED_RAM_Load; --delay by 2 clocks, 1 to read memory, 1 to register output data
            LED_RAM_Load_q <= LED_RAM_Load_d; --delay by 2 clocks, 1 to read memory, 1 to register output data
            if LED_RAM_Load_q = '1' then
                R0_Data(to_integer(unsigned(LED_RAM_Addr_int(5 downto 0))-1)) <= LED_Data_RGB_lo(to_integer(unsigned(RGB_bit_count)));
                G0_Data(to_integer(unsigned(LED_RAM_Addr_int(5 downto 0))-1)) <= LED_Data_RGB_lo(to_integer(unsigned(RGB_bit_count))+6);
                B0_Data(to_integer(unsigned(LED_RAM_Addr_int(5 downto 0))-1)) <= LED_Data_RGB_lo(to_integer(unsigned(RGB_bit_count))+12);
                R1_Data(to_integer(unsigned(LED_RAM_Addr_int(5 downto 0))-1)) <= LED_Data_RGB_hi(to_integer(unsigned(RGB_bit_count)));
                G1_Data(to_integer(unsigned(LED_RAM_Addr_int(5 downto 0))-1)) <= LED_Data_RGB_hi(to_integer(unsigned(RGB_bit_count))+6);
                B1_Data(to_integer(unsigned(LED_RAM_Addr_int(5 downto 0))-1)) <= LED_Data_RGB_hi(to_integer(unsigned(RGB_bit_count))+12);
            end if;

            if Latch_int = '1' then
                Matrix_Addr_d <= LED_RAM_Addr_int(10 downto 6);
            end if;
        end if;
    end process;
    Matrix_Addr <= Matrix_Addr_q;

    p_shift_data : process (CLK_Matrix)
    begin
        if falling_edge(CLK_Matrix) then
            if Shift_Data = '1' then
                R0 <= R0_Data(to_integer(unsigned(Shift_Data_Cntr)));
                G0 <= G0_Data(to_integer(unsigned(Shift_Data_Cntr)));
                B0 <= B0_Data(to_integer(unsigned(Shift_Data_Cntr)));
                R1 <= R1_Data(to_integer(unsigned(Shift_Data_Cntr)));
                G1 <= G1_Data(to_integer(unsigned(Shift_Data_Cntr)));
                B1 <= B1_Data(to_integer(unsigned(Shift_Data_Cntr)));
            end if;
        end if;
    end process;

    LED_RAM_Addr <= LED_RAM_Addr_int;
    LATCH <= Latch_int;

end architecture;