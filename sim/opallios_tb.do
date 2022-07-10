# Compile Design
project compileall

# Start sim
vsim -gui -L work -L ice -L matrix work.opallios_fpga_tb

# add waves
add wave -group {TB} *
add wave -group {DUT} Opallios_FPGA_tb/DUT/*
add wave -group {GPMC_sync} Opallios_FPGA_tb/DUT/u_gpmc_sync/*

# run 
run 5 us