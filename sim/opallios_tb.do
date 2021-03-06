# Compile Design
project compileall

# Start sim
vsim -gui -L work -L ice -L matrix work.opallios_fpga_tb

# add waves
add wave -group {TB} Opallios_FPGA_tb/*
add wave -group {TB} Opallios_FPGA_tb/MATRIX_TB(0)
add wave -group {TB} Opallios_FPGA_tb/MATRIX_TB(1)
add wave -group {TB} Opallios_FPGA_tb/MATRIX_TB(2)
add wave -group {DUT} Opallios_FPGA_tb/DUT/*
add wave -group {GPMC_sync} Opallios_FPGA_tb/DUT/u_gpmc_sync/*
add wave -group {Regs} Opallios_FPGA_tb/DUT/u_regs/*
add wave -group {Video Mem} Opallios_FPGA_tb/DUT/u_matrix_ram_lo/*
add wave -group {Video Mem} Opallios_FPGA_tb/DUT/u_matrix_ram_hi/*
add wave -group {Matrix IF} Opallios_FPGA_tb/DUT/u_matrix_if/*
add wave -group {Matrix SM} Opallios_FPGA_tb/DUT/u_matrix_if/u_matrix_sm/*
add wave -group {Matrix Panel} Opallios_FPGA_tb/u_matrix/*
add wave -group {Matrix Panel} Opallios_FPGA_tb/u_matrix/MATRIX_TB(0)
add wave -group {Matrix Panel} Opallios_FPGA_tb/u_matrix/MATRIX_TB(1)
add wave -group {Matrix Panel} Opallios_FPGA_tb/u_matrix/MATRIX_TB(2)
add wave -group {Matrix Sregs} Opallios_FPGA_tb/u_matrix/g_cols(0)/u_ICN2037_1/*
add wave -group {Matrix Sregs} Opallios_FPGA_tb/u_matrix/g_cols(1)/u_ICN2037_1/*
add wave -group {Matrix Sregs} Opallios_FPGA_tb/u_matrix/g_cols(2)/u_ICN2037_1/*
add wave -group {Matrix Sregs} Opallios_FPGA_tb/u_matrix/g_cols(0)/u_ICN2037_4/*
add wave -group {Matrix Sregs} Opallios_FPGA_tb/u_matrix/g_cols(1)/u_ICN2037_4/*
add wave -group {Matrix Sregs} Opallios_FPGA_tb/u_matrix/g_cols(2)/u_ICN2037_4/*

# run sim
# run 11 ms # 11ms for full speed, 22ms for halved debug speed
run 20 ms 
wave zoom full