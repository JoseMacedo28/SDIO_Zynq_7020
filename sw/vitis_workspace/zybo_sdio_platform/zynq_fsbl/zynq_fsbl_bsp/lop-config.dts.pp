#line 2 "lop-config.dts"
/dts-v1/;
/ {
        compatible = "system-device-tree-v1,lop";
        lops {
                lop_0 {
                        compatible = "system-device-tree-v1,lop,load";
                        load = "assists/baremetal_validate_comp_xlnx.py";
                };

                lop_1 {
                    compatible = "system-device-tree-v1,lop,assist-v1";
                    node = "/";
                    outdir = "/home/huezzo/Documents/LIESE/SDIO_Zynq_7020/sw/vitis_workspace/zybo_sdio_platform/zynq_fsbl/zynq_fsbl_bsp";
                    id = "module,baremetal_validate_comp_xlnx";
                    options = "ps7_cortexa9_0 /mnt/externo/Vi/Vitis/2024.2/data/embeddedsw/lib/sw_services/xilffs_v5_3/src /home/huezzo/Documents/LIESE/SDIO_Zynq_7020/sw/vitis_workspace/_ide/.wsdata/.repo.yaml";
                };

                lop_2 {
                    compatible = "system-device-tree-v1,lop,assist-v1";
                    node = "/";
                    outdir = "/home/huezzo/Documents/LIESE/SDIO_Zynq_7020/sw/vitis_workspace/zybo_sdio_platform/zynq_fsbl/zynq_fsbl_bsp";
                    id = "module,baremetal_validate_comp_xlnx";
                    options = "ps7_cortexa9_0 /mnt/externo/Vi/Vitis/2024.2/data/embeddedsw/lib/sw_services/xilrsa_v1_8/src /home/huezzo/Documents/LIESE/SDIO_Zynq_7020/sw/vitis_workspace/_ide/.wsdata/.repo.yaml";
                };

        };
    };
