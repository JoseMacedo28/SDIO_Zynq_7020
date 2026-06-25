#!/bin/sh

# 
# Vivado(TM)
# runme.sh: a Vivado-generated Runs Script for UNIX
# Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
# Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
# 

if [ -z "$PATH" ]; then
  PATH=/mnt/externo/Vi/Vitis/2024.2/bin:/mnt/externo/Vi/Vivado/2024.2/ids_lite/ISE/bin/lin64:/mnt/externo/Vi/Vivado/2024.2/bin
else
  PATH=/mnt/externo/Vi/Vitis/2024.2/bin:/mnt/externo/Vi/Vivado/2024.2/ids_lite/ISE/bin/lin64:/mnt/externo/Vi/Vivado/2024.2/bin:$PATH
fi
export PATH

if [ -z "$LD_LIBRARY_PATH" ]; then
  LD_LIBRARY_PATH=
else
  LD_LIBRARY_PATH=:$LD_LIBRARY_PATH
fi
export LD_LIBRARY_PATH

HD_PWD='/home/huezzo/Documents/LIESE/SDIO_Zynq_7020/hw/vivado_proj/zybo_sdio/zybo_sdio.runs/synth_1'
cd "$HD_PWD"

HD_LOG=runme.log
/bin/touch $HD_LOG

ISEStep="./ISEWrap.sh"
EAStep()
{
     $ISEStep $HD_LOG "$@" >> $HD_LOG 2>&1
     if [ $? -ne 0 ]
     then
         exit
     fi
}

EAStep vivado -log zybo_sdio_bd_wrapper.vds -m64 -product Vivado -mode batch -messageDb vivado.pb -notrace -source zybo_sdio_bd_wrapper.tcl
