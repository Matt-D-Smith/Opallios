# Create clock constraints
create_clock -name clk_100M -period 10.000 [get_ports {clk_100M}]
create_clock -name gpmc_clk -period 10.000 [get_ports {gpmc_clk}]
# create_generated_clock -divide_by 4 -source clk_100M [get_pins {u_matrix_interface/Clk_Div_Count_reg(1):Q}]