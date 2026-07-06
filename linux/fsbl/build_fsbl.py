#!/usr/bin/env python3
# Generates and builds the Zynq FSBL from the arty-z7-soc XSA using the
# Vitis 2026.1 Python client API (xsct-based classic project creation is
# disabled in this release -- "use the Vitis Python Console for
# script-based projects creation and build").
import vitis
import os
import shutil

script_dir = os.path.dirname(os.path.abspath(__file__))
xsa_path = os.path.join(script_dir, "..", "..", "arty-z7-soc", "hw", "arty_z7_soc.xsa")
xsa_path = os.path.abspath(xsa_path)
workspace = os.path.join(script_dir, "vitis_ws")
platform_name = "arty_z7_fsbl_platform"

if os.path.isdir(workspace):
    shutil.rmtree(workspace)

client = vitis.create_client()
client.set_workspace(workspace)

platform = client.create_platform_component(
    name=platform_name,
    hw_design=xsa_path,
    cpu="ps7_cortexa9_0",
    os="standalone",
    domain_name="standalone_a9_0",
)
platform.report()
platform.build()

fsbl_elf = None
for root, dirs, files in os.walk(workspace):
    for f in files:
        if f == "fsbl.elf" or (f.endswith(".elf") and "fsbl" in f.lower()):
            fsbl_elf = os.path.join(root, f)

if fsbl_elf:
    dest = os.path.join(script_dir, "fsbl.elf")
    shutil.copy(fsbl_elf, dest)
    print(f"FSBL_ELF_PATH={dest}")
else:
    print("FSBL_ELF_PATH=NOTFOUND")

vitis.dispose()
