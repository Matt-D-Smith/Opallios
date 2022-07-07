# Create clock constraints
create_clock -name clk_100M -period 10.000 [get_ports {clk_100M}]
create_clock -name gpmc_clk -period 10.000 [get_ports {gpmc_clk}]