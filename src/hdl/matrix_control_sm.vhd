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
        RSTn            : in  std_logic;
        Matrix_CLK_re   : in  std_logic;
        Matrix_CLK_fe   : in  std_logic;
        LED_RAM_Addr    : out std_logic_vector(10 downto 0); -- log2(64*64/2)
        Next_Frame      : out std_logic;
        Shift_Data      : out std_logic;
        Shift_Data_Cntr : out std_logic_vector(5 downto 0);
        Matrix_CLK_Gate : out std_logic;
        Blank           : out std_logic;
        Latch           : out std_logic;
        RGB_bit_count   : out std_logic_vector(2 downto 0);
        --debugging
        TP              : out std_logic_vector(7 downto 0)
    );
end matrix_control_sm;

architecture rtl of matrix_control_sm is

    signal Frame_Timer          : unsigned(26 downto 0) := (others => '0');
    signal RGB_bit_count_d      : unsigned(2 downto 0) := (others => '0');
    signal RGB_bit_count_q      : unsigned(2 downto 0) := (others => '0');

    type state_type is (Startup, Start_Shift_Data, Shift_Data_Out, Latch_Data, Output_Enable, wait_BCM);
    signal state : state_type;

    attribute syn_encoding : string;
    attribute syn_encoding of state : signal is "safe";

    signal matrix_delay_cnt         : unsigned(11 downto 0) := (others => '0'); -- max value = 64*2^6=4096 = 12 bit
    signal matrix_delay_cnt_d         : unsigned(11 downto 0) := (others => '0'); -- max value = 64*2^6=4096 = 12 bit
    signal rst_matrix_delay_cnt     : std_logic;
    signal incr_matrix_delay_cnt    : std_logic;

    signal incr_addr    : std_logic;
    signal col_addr     : unsigned(5 downto 0) := (others => '0');
    signal row_count    : unsigned(4 downto 0) := (others => '0');

    signal Shift_Data_int : std_logic;
    signal Shift_Data_Cntr_int : unsigned(5 downto 0);

begin

    p_frame_timer : process (CLK, RSTn) -- independent of drawing the frame
    begin
        if RSTn = '0' then
            Next_Frame <= '0';
            Frame_Timer <= (others => '0');
        elsif rising_edge(CLK) then
            Next_Frame <= '0';
            Frame_Timer <= Frame_Timer + 1;
            if (Frame_Timer = to_unsigned(1000000-1,Frame_Timer'length)) then -- 100 Hz
                Frame_Timer <= (others => '0');
                Next_Frame <= '1';
            end if;
        end if;
    end process;

    p_matrix_delay_counter : process (CLK, RSTn) -- count for state machine delays relative to matrix clk
    begin
        if RSTn = '0' then
            matrix_delay_cnt <= (others => '0');
            matrix_delay_cnt_d <= (others => '0');
        elsif rising_edge(CLK) then
            matrix_delay_cnt <= matrix_delay_cnt_d;
            if (Matrix_CLK_re = '1') then
                if rst_matrix_delay_cnt = '1' then 
                    matrix_delay_cnt_d <= (others => '0'); 
                elsif incr_matrix_delay_cnt = '1' then
                    matrix_delay_cnt_d <= matrix_delay_cnt_d + 1;
                end if;
            end if;
        end if;
    end process;

    p_address_counter : process (CLK, RSTn) -- count address, range 0 to 2048, as we are using half the display for addressing
    begin
        if RSTn = '0' then
            col_addr <= (others => '0');
        elsif rising_edge(CLK) then
            if (Matrix_CLK_re = '1') then
                if incr_addr = '1' then
                    col_addr <= col_addr + 1; -- roll over back to 0 when done
                end if;
            end if;
        end if;
    end process;
    LED_RAM_Addr <= std_logic_vector(row_count) & std_logic_vector(col_addr);

    p_shift_data_counter : process (CLK, RSTn)
    begin
        if RSTn = '0' then
            Shift_Data_Cntr_int <= (others => '0');
        elsif rising_edge(CLK) then
            if (Matrix_CLK_re = '1') then
                if Shift_Data_int = '1' then
                    Shift_Data_Cntr_int <= Shift_Data_Cntr_int + 1;
                else 
                    Shift_Data_Cntr_int <= (others => '0');
                end if; 
            end if;
        end if;
    end process;
    Shift_Data_Cntr <= std_logic_vector(Shift_Data_Cntr_int);

    p_next_state : process(CLK, RSTn)
    begin 
        if RSTn = '0' then
            state <= Startup;
            rst_matrix_delay_cnt <= '0';
            RGB_bit_count_d <= (others => '0');
            RGB_bit_count_q <= (others => '0');
            row_count <= (others => '0');
        elsif rising_edge(CLK) then
            RGB_bit_count_q <= RGB_bit_count_d;
            if (Matrix_CLK_re = '1') then
                -- defaults
                rst_matrix_delay_cnt <= '0';

                -- state machine
                case state is 
                    when Startup =>
                        state <= Start_Shift_Data;
                    when Start_Shift_Data =>
                        state <= Shift_Data_Out;
                    when Shift_Data_Out =>  
                        if matrix_delay_cnt = to_unsigned(63,matrix_delay_cnt'length) then
                            state <= Latch_Data;
                            rst_matrix_delay_cnt <= '1';
                        end if;
                    when Latch_Data =>  
                        state <= Output_Enable;
                    when Output_Enable =>  
                        state <= wait_BCM;
                    when wait_BCM =>  
                        if matrix_delay_cnt = shift_left(to_unsigned(64,matrix_delay_cnt'length),to_integer(RGB_bit_count_q))-64 then -- lsh +6 for 64x to make the 64 clock shift period the unit delay (approx)
                            if RGB_bit_count_q = to_unsigned(5,RGB_bit_count_q'length) then
                                state <= Start_Shift_Data;
                                RGB_bit_count_d <= (others => '0');
                                row_count <= row_count + 1;
                            else
                                state <= Start_Shift_Data;
                                RGB_bit_count_d <= RGB_bit_count_q + 1; -- roll back over when done
                            end if;
                            rst_matrix_delay_cnt <= '1';
                        end if;
                    when others =>
                        state <= Start_Shift_Data;
                end case; 
            end if;
        end if;
    end process;

    p_state_signals : process(state)
    begin 
        -- defaults
        Latch <= '0';
        Blank <= '0';
        incr_addr <= '0';
        Shift_Data_int <= '0';
        incr_matrix_delay_cnt <= '0';
        Matrix_CLK_Gate <= '0';

        -- state machine
        case state is 
            when Startup =>
            when Start_Shift_Data =>
                Shift_Data_int <= '1'; 
                incr_addr <= '1';
            when Shift_Data_Out =>  
                Shift_Data_int <= '1';
                incr_addr <= '1';
                Matrix_CLK_Gate <= '1';
                incr_matrix_delay_cnt <= '1';
            when Latch_Data =>  
                Latch <= '1';
            when Output_Enable =>  
                Blank <= '1';
            when wait_BCM =>  
                incr_matrix_delay_cnt <= '1';
            when others => 
        end case; 
    end process;

    RGB_bit_count <= std_logic_vector(RGB_bit_count_q);
    Shift_Data <= Shift_Data_int;

    -- debug
    -- TP(0) <= '1' when state = Startup else '0';
    -- TP(1) <= '1' when state = Load_Row_RAM_Data else '0';
    -- TP(2) <= '1' when state = Shift_Data_Out else '0';
    -- TP(3) <= '1' when state = Latch_Data else '0';
    -- TP(4) <= '1' when state = wait_BCM else '0';
    -- TP(5) <= Matrix_CLK_re;
    -- TP(6) <= Shift_Data_int;
    -- TP(7) <= incr_matrix_delay_cnt;
    TP <= (others => '0');

end architecture;