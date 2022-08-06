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
    generic (
        DEBUG : boolean := false
    );
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
        LATCH       : out std_logic;
        TP          : out std_logic_vector(7 downto 0)
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
            gpmc_wen    : in std_logic;
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
        generic (
            DEBUG : boolean := false
        );
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
    signal we_matrix_lo     : std_logic;
    signal we_matrix_hi     : std_logic;
    signal we_matrix_buf    : std_logic;
    signal LED_Wr_Addr      : std_logic_vector(10 downto 0); -- log2(64*64) bits
    signal LED_Rd_Addr      : std_logic_vector(10 downto 0); -- log2(64*64) bits
    signal LED_Data_RG_D    : std_logic_vector(11 downto 0);
    signal LED_Data_RG_Q    : std_logic_vector(11 downto 0);
    signal LED_Wr_Data_RGB  : std_logic_vector(17 downto 0); -- 18 bit color
    signal LED_Data_RGB_lo  : std_logic_vector(17 downto 0); -- 18 bit color
    signal LED_Data_RGB_hi  : std_logic_vector(17 downto 0); -- 18 bit color
    signal LED_Data_RGB_lo_q: std_logic_vector(17 downto 0); -- register ram data to help timing
    signal LED_Data_RGB_hi_q: std_logic_vector(17 downto 0); -- register ram data to help timing

    -- Reset
    signal RSTn_counter    : std_logic_vector(15 downto 0) := (others => '0');
    signal RSTn      : std_logic := '0';

    attribute syn_preserve : boolean;
    attribute syn_preserve of RSTn_counter : signal is true;

    -- debug internal signals
    signal R0_int : std_logic;
    signal G0_int : std_logic;
    signal B0_int : std_logic;
    signal BLANK_int : std_logic;
    signal LATCH_int : std_logic;
    signal Matrix_CLK_int : std_logic;
    signal Matrix_if_TP :std_logic_vector(7 downto 0);

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
        gpmc_wen    => gpmc_wen,
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

    LED_Wr_Addr <= gpmc_addr(11 downto 1); -- divide by 2 to map 8192 to 4096
    we_matrix_buf <= we when (gpmc_addr >= S_MATRIX_ADDR) and (gpmc_addr <= E_MATRIX_ADDR) else '0';
    we_matrix_lo <= we_matrix_buf when (gpmc_addr(0) = '1') and (gpmc_addr(12) = '0') else '0'; -- only enable we when writing to the blue data reg, lo regs
    we_matrix_hi <= we_matrix_buf when (gpmc_addr(0) = '1') and (gpmc_addr(12) = '1') else '0'; -- only enable we when writing to the blue data reg, hi regs

    p_RG_reg: process (clk_100M)
    begin
        if rising_edge(clk_100M) then
            LED_Data_RG_Q <= LED_Data_RG_D;
            if ((we_matrix_buf = '1') and (gpmc_addr(0) = '0')) then
                LED_Data_RG_D <= data_wr(13 downto 8) & data_wr(5 downto 0); -- divide R and G to lower and upper byte
            end if;
        end if;
    end process;

    LED_Wr_Data_RGB <= data_wr(5 downto 0) & LED_Data_RG_Q;

    u_matrix_ram_lo : dual_port_ram -- store lower address data
    generic map (
        addr_width => 11, --2096x18
        data_width => 18
    )
    port map (
        write_en    => we_matrix_lo,
        waddr       => LED_Wr_Addr,
        wclk        => clk_100M,
        raddr       => LED_Rd_Addr,
        rclk        => clk_100M,
        din         => LED_Wr_Data_RGB,
        dout        => LED_Data_RGB_lo
    );

    u_matrix_ram_hi : dual_port_ram -- store upper address data
    generic map (
        addr_width => 11, --2096x18
        data_width => 18
    )
    port map (
        write_en    => we_matrix_hi,
        waddr       => LED_Wr_Addr,
        wclk        => clk_100M,
        raddr       => LED_Rd_Addr,
        rclk        => clk_100M,
        din         => LED_Wr_Data_RGB,
        dout        => LED_Data_RGB_hi
    );

    u_matrix_if: matrix_interface
    generic map (
        DEBUG => DEBUG
    )
    port map (
        CLK             => CLK_100M,
        RSTn            => RSTn,
        LED_Data_RGB_lo => LED_Data_RGB_lo,
        LED_Data_RGB_hi => LED_Data_RGB_hi,
        LED_RAM_Addr    => LED_Rd_Addr,
        R0              => R0_int,
        G0              => G0_int,
        B0              => B0_int,
        R1              => R1,
        G1              => G1,
        B1              => B1,
        Matrix_Addr     => Matrix_Addr,
        Matrix_CLK      => Matrix_CLK_int,
        BLANK           => BLANK_int,
        LATCH           => LATCH_int,
        Next_Frame      => open,
        TP              => matrix_if_TP
    );

    p_reset_gen : process (CLK_100M)
    begin
        if rising_edge(CLK_100M) then
            RSTn_counter   <= RSTn_counter(14 downto 0) & '1';
            RSTn     <= RSTn_counter(15);
        end if;
    end process;

    -- debug outputs
    Matrix_CLK <= Matrix_CLK_int;
    R0    <= R0_int;
    G0    <= G0_int;
    B0    <= B0_int;
    BLANK <= BLANK_int;
    LATCH <= LATCH_int;

    P_tp_reg : process (CLK_100M)
    begin
        if rising_edge(CLK_100M) then
            -- TP(0) <= Matrix_CLK_int;
            -- TP(1) <= R0_int;
            -- TP(2) <= G0_int;
            -- TP(3) <= B0_int;
            -- TP(4) <= BLANK_int;
            -- TP(5) <= LATCH_int;
            -- TP(6) <= matrix_if_TP(6);
            -- TP(7) <= matrix_if_TP(7);
            TP(4 downto 0) <= matrix_if_TP(4 downto 0);
            TP(5) <= BLANK_int;
            TP(6) <= LATCH_int;
            TP(7) <= '0';
        end if;
    end process;

end architecture;