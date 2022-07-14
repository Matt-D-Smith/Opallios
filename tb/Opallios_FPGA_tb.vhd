--------------------------------------------------------------------------------
-- Project      : Opallios
--------------------------------------------------------------------------------
-- File         : Opallios_FPGA_tb.vhd
-- Generated    : 07/06/2022
--------------------------------------------------------------------------------
-- Description  : Testbench for Opallios FPGA 
--------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity Opallios_FPGA_tb is
end entity Opallios_FPGA_tb;

architecture rtl of Opallios_FPGA_tb is

    -- BeagleWire signals
    signal clk_100M    : std_logic;
    signal led         : std_logic_vector(3 downto 0);
    signal btn         : std_logic_vector(1 downto 0);
    signal sw          : std_logic_vector(1 downto 0);
    -- GPMC Interface
    signal gpmc_ad     : std_logic_vector(15 downto 0);
    signal gpmc_advn   : std_logic;
    signal gpmc_csn1   : std_logic;
    signal gpmc_wen    : std_logic;
    signal gpmc_oen    : std_logic;
    signal gpmc_clk    : std_logic;
    -- HUB75 interface
    signal R0          : std_logic;
    signal G0          : std_logic;
    signal B0          : std_logic;
    signal R1          : std_logic;
    signal G1          : std_logic;
    signal B1          : std_logic;
    signal Matrix_Addr : std_logic_vector(4 downto 0);
    signal Matrix_CLK  : std_logic;
    signal BLANK       : std_logic;
    signal LATCH       : std_logic;

    -- Clock period definitions
    constant GPMC_CLK_period : time := 10 ns;

    component led_matrix_fpga_top is
        port (
            -- BeagleWire signals
            clk_100M    : in  std_logic;
            led         : out std_logic_vector(3 downto 0);
            btn         : in  std_logic_vector(1 downto 0);
            sw          : in  std_logic_vector(1 downto 0);
            -- GPMC Interface
            gpmc_ad     : inout  std_logic_vector(15 downto 0);
            gpmc_advn   : in  std_logic;
            gpmc_csn1   : in  std_logic;
            gpmc_wen    : in  std_logic;
            gpmc_oen    : in  std_logic;
            gpmc_clk    : in  std_logic;
            -- HUB75 interface
            R0          : out std_logic;
            G0          : out std_logic;
            B0          : out std_logic;
            R1          : out std_logic;
            G1          : out std_logic;
            B1          : out std_logic;
            Matrix_Addr : out std_logic_vector(4 downto 0);
            Matrix_CLK  : out std_logic;
            BLANK       : out std_logic;
            LATCH       : out std_logic
        );
    end component;

    
        

begin

    --TB signals
    p_clk_100M : process
    begin
        clk_100M <= '1';
        wait for 5 ns;
        clk_100M <= '0';
        wait for 5 ns;
    end process;

    DUT: led_matrix_fpga_top
        port map (
            -- BeagleWire signals
            clk_100M    => clk_100M,
            led         => led,
            btn         => btn,
            sw          => sw,
            -- GPMC Interface
            gpmc_ad     => gpmc_ad,
            gpmc_advn   => gpmc_advn,
            gpmc_csn1   => gpmc_csn1,
            gpmc_wen    => gpmc_wen,
            gpmc_oen    => gpmc_oen,
            gpmc_clk    => gpmc_clk,
            -- HUB75 interface
            R0          => R0,
            G0          => G0,
            B0          => B0,
            R1          => R1,
            G1          => G1,
            B1          => B1,
            Matrix_Addr => Matrix_Addr,
            Matrix_CLK  => Matrix_CLK,
            BLANK       => BLANK,
            LATCH       => LATCH
        );

    -- Clock process definitions
    p_GPMC_CLK :process
    begin
        GPMC_CLK <= '0';
        wait for GPMC_CLK_period/2;
        GPMC_CLK <= '1';
        wait for GPMC_CLK_period/2;
    end process;

    -- Stimulus process
    stim_proc: process
    procedure gpmc_send (RW   : std_logic;
                         ADDR : std_logic_vector(15 downto 0);
                         DATA : std_logic_vector(15 downto 0)) is
    begin
        
        GPMC_AD <= ADDR ;
        GPMC_CSN1 <= '0' ;
        GPMC_ADVN <= '0' ;
        GPMC_OEN <= '1' ;
        GPMC_WEN <= '1' ;
        wait for GPMC_CLK_period*2;
        GPMC_AD <= (others => 'Z') ;
        GPMC_CSN1 <= '0' ;
        GPMC_ADVN <= '1' ;
        GPMC_OEN <= '1' ;
        GPMC_WEN <= '1' ;
        wait for GPMC_CLK_period;
        if RW = '1' then 
            GPMC_AD <= DATA ;
            GPMC_CSN1 <= '0' ;
            GPMC_ADVN <= '1' ;
            GPMC_OEN <= '1' ;
            GPMC_WEN <= '0' ;     
        else 
            GPMC_AD <= (others => 'Z') ;
            GPMC_CSN1 <= '0' ;
            GPMC_ADVN <= '1' ;
            GPMC_OEN <= '0' ;
            GPMC_WEN <= '1' ;
        end if;
        wait for GPMC_CLK_period*4;
        GPMC_AD <= (others => 'Z') ;
        GPMC_CSN1 <= '1' ;
        GPMC_ADVN <= '1' ;
        GPMC_OEN <= '1' ;
        GPMC_WEN <= '1' ;
        wait for GPMC_CLK_period*10;
    end procedure gpmc_send;

    begin

        GPMC_AD <=  (others => 'Z');
        GPMC_CSN1 <= '1' ;
        GPMC_ADVN <= '1' ;
        GPMC_OEN <= '1' ;
        GPMC_WEN <= '1' ;

    -- wait for 100 ns;	
    -- wait for GPMC_CLK_period*2000;	
    wait for GPMC_CLK_period*10;
    -- gpmc_send format - RW: 0 read 1 write, ADDR, DATA
    -- read reg 0
    gpmc_send('0',x"0000",x"0000");
    -- write reg 0
    gpmc_send('1',x"0000",x"1234");
    -- read reg 0
    gpmc_send('0',x"0000",x"0000");
    -- read reg 1
    gpmc_send('0',x"0001",x"0000");
    -- read reg 0
    gpmc_send('0',x"0000",x"0000");

    -- Write some data to the video memory
    gpmc_send('1',x"2000",x"02AA");
    gpmc_send('1',x"2001",x"0015");
    gpmc_send('1',x"2002",x"0155");
    gpmc_send('1',x"2003",x"002A");
    gpmc_send('1',x"2004",x"00CC");
    gpmc_send('1',x"2005",x"0033");
    gpmc_send('1',x"2006",x"0333");
    gpmc_send('1',x"2007",x"000C");


    wait;
    end process;
    -- insert stimulus here 
    

end architecture;