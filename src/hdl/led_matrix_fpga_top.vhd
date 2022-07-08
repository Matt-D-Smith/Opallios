--------------------------------------------------------------------------------
-- Project      : Opallios
--------------------------------------------------------------------------------
-- File         : led_matrix_fpga_top.vhd
-- Generated    : 07/06/2022
--------------------------------------------------------------------------------
-- Description  : Top level Beaglewire FPGA interface for driving 64x64 LED matrix
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
        sw          : in  std_logic_vector(1 downto 0);
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
            CLK             : in  std_logic;
            LED_Addr        : out std_logic_vector(11 downto 0); -- log2(64*64)
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

    -- S_ for start range, E_ for end range, R_ for register
    constant S_REGS_ADDR    : std_logic_vector := x"0000"; -- map 16x16 register space
    constant E_REGS_ADDR    : std_logic_vector := x"000F";
    constant S_MATRIX_ADDR  : std_logic_vector := x"2000"; -- map 4096x18 to 8192x16
    constant E_MATRIX_ADDR  : std_logic_vector := x"3FFF";

    -- GPMC constants
    constant GPMC_ADDR_WIDTH    : integer := 16;
    constant GPMC_DATA_WIDTH    : integer := 16;
    constant RAM_DEPTH          : integer := 2**GPMC_ADDR_WIDTH;
    -- GPMC signals
    signal oen              : std_logic;
    signal wen              : std_logic;
    signal csn              : std_logic;
    signal gpmc_addr        : std_logic_vector(GPMC_ADDR_WIDTH-1 downto 0);
    signal data_wr          : std_logic_vector(GPMC_DATA_WIDTH-1 downto 0);
    signal data_rd          : std_logic_vector(GPMC_DATA_WIDTH-1 downto 0);
    
    -- reg ram signals
    signal oe               : std_logic;
    signal we               : std_logic;
    signal we_regs          : std_logic;
    signal read_addr        : std_logic_vector(GPMC_ADDR_WIDTH-1 downto 0);
    signal raddr            : std_logic_vector(GPMC_ADDR_WIDTH-1 downto 0);
    -- matrix LED ram signals
    signal we_matrix        : std_logic;
    signal we_matrix_buf    : std_logic;
    signal LED_Wr_Addr      : std_logic_vector(11 downto 0); -- log2(64*64) bits
    signal LED_Rd_Addr      : std_logic_vector(11 downto 0); -- log2(64*64) bits
    signal LED_Data_RG_D    : std_logic_vector(11 downto 0);
    signal LED_Data_RG_Q    : std_logic_vector(11 downto 0);
    signal LED_Data_RGB     : std_logic_vector(17 downto 0);

    -- Register map
    signal frame_status : std_logic_vector(15 downto 0); -- TBD contents

    -- matrix interface
    -- this may need to change, frame sync signals and memory interface 
    signal Frame_Addr   : std_logic_vector(11 downto 0); -- log2(64*64) bits
    signal Frame_data   : std_logic_vector(17 downto 0); -- 18 bit color

begin

    --temporary output assignments
    led <= (others => '0'); 

    u_gpmc_sync: gpmc_sync
    generic map (
        DATA_WIDTH => GPMC_DATA_WIDTH,
        ADDR_WIDTH => GPMC_ADDR_WIDTH
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
        data_out    => data_wr,
        data_in     => data_rd
    );

    oe <= (not csn) and wen and (not oen); -- this may need to add a when for FPGA side writes
    we <= (not csn) and (not wen) and oen; -- this may need to add a when for FPGA side writes
    we_regs <= we when (gpmc_addr >= S_REGS_ADDR) and (gpmc_addr <= E_REGS_ADDR) else '0';
    read_addr <= (others => '0'); -- zero for now, FPGA side reads later 
    raddr <= gpmc_addr when oe = '1' else read_addr;

    u_regs : dual_port_ram
    generic map (
        addr_width => 4, --16x16
        data_width => 16
    )
    port map (
        write_en    => we_regs,
        waddr       => gpmc_addr(3 downto 0),
        wclk        => clk_100M,
        raddr       => raddr(3 downto 0),
        rclk        => clk_100M,
        din         => data_wr,
        dout        => data_rd
    );

    LED_Wr_Addr <= gpmc_addr(12 downto 1); -- divide by 2 to map 8192 to 4096
    we_matrix_buf <= we when (gpmc_addr >= S_MATRIX_ADDR) and (gpmc_addr <= E_MATRIX_ADDR) else '0';
    we_matrix <= we_matrix_buf when (gpmc_addr(0) = '1') else '0'; -- only enable we when writing to the blue data reg

    p_RG_reg: process (clk_100M)
    begin
        if rising_edge(clk_100M) then
            LED_Data_RG_Q <= LED_Data_RG_D;
            if ((we_matrix_buf = '1') and (gpmc_addr(0) = '0')) then
                LED_Data_RG_D <= data_wr(11 downto 0);
            end if;
        end if;
    end process;

    u_matrix_ram : dual_port_ram
    generic map (
        addr_width => 12, --4096x18
        data_width => 18
    )
    port map (
        write_en    => we_matrix,
        waddr       => LED_Wr_Addr,
        wclk        => clk_100M,
        raddr       => LED_Rd_Addr,
        rclk        => clk_100M,
        din         => data_wr(5 downto 0) & LED_Data_RG_Q,
        dout        => LED_Data_RGB
    );

    u_matrix_if: matrix_interface
    port map (
        CLK             => CLK_100M,
        LED_Addr        => LED_Rd_Addr,
        LED_Data_RGB    => LED_Data_RGB,
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