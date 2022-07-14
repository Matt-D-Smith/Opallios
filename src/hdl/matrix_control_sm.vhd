--------------------------------------------------------------------------------
-- Project      : Opallios
--------------------------------------------------------------------------------
-- File         : matrix_control_sm.vhd
-- Generated    : 07/08/2022
--------------------------------------------------------------------------------
-- Description  : 64x64 LED matrix control state machine
--------------------------------------------------------------------------------
library ieee;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;

entity matrix_control_sm IS
    port (
        CLK             : in  std_logic;
        CLK_Matrix      : in  std_logic;
        LED_RAM_Addr    : out std_logic_vector(10 downto 0); -- log2(64*64/2)
        LED_RAM_Load    : out std_logic;
        Next_Frame      : out std_logic;
        Shift_Data      : out std_logic;
        Blank           : out std_logic;
        Latch           : out std_logic;
        RGB_bit_count   : out std_logic_vector(2 downto 0)
    );
end matrix_control_sm;

architecture rtl of matrix_control_sm is

    signal Frame_Timer          : unsigned(26 downto 0) := (others => '0');
    signal RGB_bit_count_int    : unsigned(2 downto 0) := (others => '0');

    type state_type is (Startup, Load_Row_RAM_Data, Shift_Data_Out, Latch_Data, wait_BCM, wait_row);
    signal state, next_state : state_type;

    signal ram_delay_cnt        : unsigned(7 downto 0) := (others => '0');
    signal rst_ram_delay_cnt    : std_logic;
    signal incr_ram_delay_cnt   : std_logic;

    signal matrix_delay_cnt         : unsigned(11 downto 0) := (others => '0'); -- max value = 64*2^6=4096 = 12 bit
    signal rst_matrix_delay_cnt     : std_logic;
    signal incr_matrix_delay_cnt    : std_logic;

    signal incr_addr    : std_logic;
    signal col_addr     : unsigned(5 downto 0) := (others => '0');
    signal row_count    : unsigned(4 downto 0) := (others => '0');

begin

    p_frame_timer : process (CLK) -- independent of drawing the frame
    begin
        if rising_edge(CLK) then
            Next_Frame <= '0';
            Frame_Timer <= Frame_Timer + 1;
            if (Frame_Timer = to_unsigned(1000000-1,Frame_Timer'length)) then -- 100 Hz
                Frame_Timer <= (others => '0');
                Next_Frame <= '1';
            end if;
        end if;
    end process;

    p_ram_delay_counter : process (CLK) -- count for loading RAM into row data buffer
    begin
        if rising_edge(CLK) then
            if rst_ram_delay_cnt = '1' then 
                ram_delay_cnt <= (others => '0'); 
            elsif incr_ram_delay_cnt = '1' then
                ram_delay_cnt <= ram_delay_cnt + 1;
            end if;
        end if;
    end process;

    p_matrix_delay_counter : process (CLK_Matrix) -- count for state machine delays relative to matrix clk
    begin
        if rising_edge(CLK_Matrix) then
            if rst_matrix_delay_cnt = '1' then 
                matrix_delay_cnt <= (others => '0'); 
            elsif incr_matrix_delay_cnt = '1' then
                matrix_delay_cnt <= matrix_delay_cnt + 1;
            end if;
        end if;
    end process;

    p_address_counter : process (CLK) -- count address, range 0 to 2048, as we are using half the display for addressing
    begin
        if rising_edge(CLK) then
            if incr_addr = '1' then
                col_addr <= col_addr + 1; -- roll over back to 0 when done
            end if;
        end if;
    end process;
    LED_RAM_Addr <= std_logic_vector(row_count) & std_logic_vector(col_addr);
    LED_RAM_Load <= incr_addr;

    p_next_state : process(CLK_Matrix)
    begin 
        if rising_edge(CLK_Matrix) then
            -- defaults
            rst_ram_delay_cnt <= '0';
            rst_matrix_delay_cnt <= '0';

            -- next state
            state <= next_state;

            -- state machine
            case state is 
                when Startup =>
                    next_state <= Load_Row_RAM_Data;
                when Load_Row_RAM_Data =>
                    if ram_delay_cnt >= to_unsigned(127-4,ram_delay_cnt'length) then -- 128 CLK cycles (-4 as 1 Matrix_CLK delay to next state)
                        next_state <= Shift_Data_Out;
                        rst_ram_delay_cnt <= '1';
                    end if;
                when Shift_Data_Out =>  
                    if matrix_delay_cnt = to_unsigned(63-1,matrix_delay_cnt'length) then -- -1 for next state delay
                        next_state <= Latch_Data;
                        rst_matrix_delay_cnt <= '1';
                    end if;
                when Latch_Data =>  
                    next_state <= wait_BCM;
                when wait_BCM =>  
                    if matrix_delay_cnt = shift_left(to_unsigned(1,matrix_delay_cnt'length),to_integer(RGB_bit_count_int)+6)-63 then -- lsh +6 for 64x to make the 64 clock shift period the unity delay (approx)
                        if RGB_bit_count_int = to_unsigned(6,RGB_bit_count_int'length) then
                            next_state <= Load_Row_RAM_Data;
                            RGB_bit_count_int <= (others => '0');
                            row_count <= row_count + 1;
                        else
                            next_state <= Shift_Data_Out;
                            RGB_bit_count_int <= RGB_bit_count_int + 1; -- roll back over when done
                        end if;
                        rst_matrix_delay_cnt <= '1';
                    end if;
                when others =>
                    next_state <= Load_Row_RAM_Data;
            end case; 
        end if;
    end process;

    p_state_signals : process(state)
    begin 
        -- defaults
        Latch <= '0';
        Blank <= '0';
        incr_addr <= '0';
        incr_ram_delay_cnt <= '0';
        Shift_Data <= '0';
        incr_matrix_delay_cnt <= '0';

        -- state machine
        case state is 
            when Startup =>
            when Load_Row_RAM_Data =>
                incr_addr <= '1';
                incr_ram_delay_cnt <= '1';
            when Shift_Data_Out =>  
                Shift_Data <= '1';
                incr_matrix_delay_cnt <= '1';
            when Latch_Data =>  
                Latch <= '1';
                Blank <= '1';
            when wait_BCM =>  
                incr_matrix_delay_cnt <= '1';
            when others => 
        end case; 
    end process;

    RGB_bit_count <= std_logic_vector(RGB_bit_count_int);

end architecture;