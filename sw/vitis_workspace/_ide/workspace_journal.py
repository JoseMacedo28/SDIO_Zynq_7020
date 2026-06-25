# 2026-06-25T03:09:49.398907
import vitis

client = vitis.create_client()
client.set_workspace(path="vitis_workspace")

platform = client.get_component(name="zybo_sdio_platform")
status = platform.build()

comp = client.create_app_component(name="sdio_polling",platform = "$COMPONENT_LOCATION/../zybo_sdio_platform/export/zybo_sdio_platform/zybo_sdio_platform.xpfm",domain = "standalone_ps7_cortexa9_0")

domain = platform.get_domain(name="standalone_ps7_cortexa9_0")

status = domain.set_lib(lib_name="xilffs", path="/mnt/externo/Vi/Vitis/2024.2/data/embeddedsw/lib/sw_services/xilffs_v5_3")

status = domain.regenerate()

