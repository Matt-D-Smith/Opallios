--------------------------------------------------------------------------------
-- Project      : Opallios
--------------------------------------------------------------------------------
-- File         : led_matrix_fpga_top.vhd
-- Generated    : 06/19/2022
--------------------------------------------------------------------------------
-- Description  : Top level Beaglewire FPGA interface for driving 64x64 LED matrix
--------------------------------------------------------------------------------
-- Revision History :
--   Date        Author        Changes
--   06/19/2022  Matt D Smith  Initial Design
--------------------------------------------------------------------------------
library ieee;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;

entity led_matrix_fpga_top is
    port (
        -- BeagleWire signals
        clk_100M    : in  std_logic;
        led         : out std_logic_vector(3 downto 0);
        btn         : in  std_logic_vector(1 downto 0);
        -- GPMC Interface
        gpmc_ad     : inout  std_logic_vector(15 downto 0);
        gpmc_advn   : in  std_logic;
        gpmc_csn1   : in  std_logic;
        gpmc_wein   : in  std_logic;
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
end entity;

architecture rtl of led_matrix_fpga_top is

    component gpmc_sync is
        generic (
            DATA_WIDTH : integer := 16;
            ADDR_WIDTH : integer := 16
        );
        port (
            clk         : in std_logic;

            gpmc_ad     : inout std_logic_vector(15 downto 0);
            gpmc_advn   : in std_logic;
            gpmc_csn1   : in std_logic;
            gpmc_wein   : in std_logic;
            gpmc_oen    : in std_logic;
            gpmc_clk    : in std_logic;

            oe          : out std_logic;
            we          : out std_logic;
            cs          : out std_logic;
            address     : out std_logic_vector(ADDR_WIDTH-1 downto 0);
            data_out    : out std_logic_vector(DATA_WIDTH-1 downto 0);
            data_in     : in  std_logic_vector(DATA_WIDTH-1 downto 0)
        );
    end component;

    component matrix_interface is
        port (
            CLK             : in std_logic;
            Frame_Addr      : in std_logic_vector(11 downto 0); -- log2(64*64)
            Frame_data      : in std_logic_vector(17 downto 0); -- 18 bit color
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
    end component;

    component dual_port_ram is
        generic (
            addr_width : natural := 9;--512x8
            data_width : natural := 8
        );
        port (
            write_en    : in std_logic;
            waddr       : in std_logic_vector (addr_width - 1 downto 0);
            wclk        : in std_logic;
            raddr       : in std_logic_vector (addr_width - 1 downto 0);
            rclk        : in std_logic;
            din         : in std_logic_vector (data_width - 1 downto 0);
            dout        : out std_logic_vector (data_width - 1 downto 0)
        );
    end component;

    -- GPMC interface signals
    constant ADDR_WIDTH : integer := 4;
    constant DATA_WIDTH : integer := 16;
    constant RAM_DEPTH  : integer := 2**ADDR_WIDTH;

    -- GPMC signals
    signal oen          : std_logic;
    signal wen          : std_logic;
    signal csn          : std_logic;
    signal gpmc_addr    : std_logic_vector(ADDR_WIDTH-1 downto 0);
    signal data_out     : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal data_in      : std_logic_vector(DATA_WIDTH-1 downto 0);
    -- DP ram signals
    signal oe           : std_logic;
    signal we           : std_logic;
    signal read_addr    : std_logic_vector(ADDR_WIDTH-1 downto 0);
    signal raddr        : std_logic_vector(ADDR_WIDTH-1 downto 0);

    -- Register map
    signal frame_status : std_logic_vector(15 downto 0); -- TBD contents
    signal LED_address  : std_logic_vector(11 downto 0); -- log2(64*64) bits
    signal LED_Data_RG  : std_logic_vector(11 downto 0);
    signal LED_Data_B   : std_logic_vector(5 downto 0);

    -- matrix interface
    -- this may need to change, frame sync signals and memory interface 
    signal Frame_Addr   : std_logic_vector(11 downto 0); -- log2(64*64) bits
    signal Frame_data   : std_logic_vector(17 downto 0); -- 18 bit color

begin

    oe <= (not csn) and wen and (not oen); -- this may need to add a when for FPGA side writes
    we <= (not csn) and (not wen) and oen; -- this may need to add a when for FPGA side writes
    read_addr <= (others => '0'); -- zero for now, FPGA side reads later 
    raddr <= gpmc_addr when oe = '1' else read_addr;

    u_mem : dual_port_ram
    generic map (
        addr_width => 4, --16x16
        data_width => 16
    )
    port map (
        write_en    => we,
        waddr       => gpmc_addr,
        wclk        => clk_100M,
        raddr       => raddr,
        rclk        => clk_100M,
        din         => data_out,
        dout        => data_in
    );

    u_gpmc_sync: gpmc_sync
    generic map (
        DATA_WIDTH => DATA_WIDTH,
        ADDR_WIDTH => ADDR_WIDTH
    )
    port map (
        clk         => clk_100M,

        gpmc_ad     => gpmc_ad,
        gpmc_advn   => gpmc_advn,
        gpmc_csn1   => gpmc_csn1,
        gpmc_wein   => gpmc_wein,
        gpmc_oen    => gpmc_oen,
        gpmc_clk    => gpmc_clk,

        oe          => oen,
        we          => wen,
        cs          => csn,
        address     => gpmc_addr,
        data_out    => data_out,
        data_in     => data_in
    );

    u_matrix_if: matrix_interface
    port map (
        CLK             => CLK_100M,
        Frame_Addr      => Frame_Addr, -- this may need to change
        Frame_Data      => Frame_Data, -- this may need to change
        R0              => R0,
        G0              => G0,
        B0              => B0,
        R1              => R1,
        G1              => G1,
        B1              => B1,
        Matrix_Addr     => Matrix_Addr,
        Matrix_CLK      => Matrix_CLK,
        BLANK           => BLANK,
        LATCH           => LATCH
    );

end architecture;