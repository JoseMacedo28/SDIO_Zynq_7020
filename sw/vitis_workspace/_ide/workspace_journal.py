# 2026-06-25T17:28:55.052211
import vitis

client = vitis.create_client()
client.set_workspace(path="vitis_workspace")

platform = client.get_component(name="zybo_sdio_platform")
status = platform.build()

comp = client.get_component(name="sdio_polling")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

comp = client.create_app_component(name="sdio_polling_v2",platform = "$COMPONENT_LOCATION/../zybo_sdio_platform/export/zybo_sdio_platform/zybo_sdio_platform.xpfm",domain = "standalone_ps7_cortexa9_0")

status = platform.build()

comp = client.get_component(name="sdio_polling_v2")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

domain = platform.get_domain(name="standalone_ps7_cortexa9_0")

status = domain.set_config(option = "lib", param = "XILFFS_use_lfn", value = "1", lib_name="xilffs")

status = platform.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

