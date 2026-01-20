transcript on
if {[file exists rtl_work]} {
	vdel -lib rtl_work -all
}
vlib rtl_work
vmap work rtl_work


vlog -sv -work work +incdir+/root/ossc/rtl/nextasic {/root/ossc/rtl/nextasic/microphone.v}
vlog -sv -work work +incdir+/root/ossc/rtl/nextasic {/root/ossc/rtl/nextasic/data_sync.v}
vlog -sv -work work +incdir+/root/ossc/rtl/nextasic {/root/ossc/rtl/nextasic/util.v}
vlog -sv -work work +incdir+/root/ossc/rtl/nextasic/spi_lib {/root/ossc/rtl/nextasic/spi_lib/spi_slave.v}
vlog -sv -work work +incdir+/root/ossc/rtl/nextasic {/root/ossc/rtl/nextasic/spi_receiver.v}
vlog -sv -work work +incdir+/root/ossc/rtl/nextasic/mlaw {/root/ossc/rtl/nextasic/mlaw/LIN2MLAW.v}

vsim -t 1ps -L altera_ver -L lpm_ver -L sgate_ver -L altera_mf_ver -L altera_lnsim_ver -L cycloneive_ver -L rtl_work -L work -voptargs="+acc"  test_Delay

add wave *
view structure
view signals
run -all
