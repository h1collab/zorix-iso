#!/usr/bin/env python3
from pathlib import Path
import sys
import yaml

ROOT = Path(__file__).resolve().parents[1]
CAL = ROOT / "calamares"
MOD = CAL / "modules"

def fail(msg):
    print("ERROR:", msg, file=sys.stderr)
    raise SystemExit(1)

settings = yaml.safe_load((CAL / "settings.conf").read_text())
if not isinstance(settings, dict):
    fail("settings.conf is not a mapping")

instances = {}
for item in settings.get("instances", []):
    if not isinstance(item, dict) or "id" not in item or "module" not in item:
        fail("invalid module instance entry")
    instances[item["id"]] = item
    cfg = item.get("config")
    if cfg and not (MOD / cfg).is_file():
        fail(f"missing config for instance {item['id']}: {cfg}")

exec_modules = []
for phase in settings.get("sequence", []):
    if isinstance(phase, dict):
        for entry in phase.get("exec", []):
            exec_modules.append(entry)

required_order = [
    "partition", "mount", "unpackfs", "machineid", "shellprocess@preuser",
    "locale", "keyboard", "localecfg", "fstab", "initramfs", "users",
    "displaymanager", "services-systemd", "shellprocess@postinstall",
    "bootloader", "umount"
]
positions = {}
for name in required_order:
    try:
        positions[name] = exec_modules.index(name)
    except ValueError:
        fail(f"required install module missing from exec sequence: {name}")

for a, b in zip(required_order, required_order[1:]):
    if positions[a] >= positions[b]:
        fail(f"install module order is unsafe: {a} must precede {b}")

partition = yaml.safe_load((MOD / "partition.conf").read_text())
if partition.get("defaultPartitionTableType") != "gpt":
    fail("partition.conf must default to GPT")
if partition.get("efi", {}).get("mountPoint") != "/boot/efi":
    fail("EFI mount point must be /boot/efi")

boot = yaml.safe_load((MOD / "bootloader.conf").read_text())
if boot.get("efiBootLoader") != "grub":
    fail("UEFI bootloader must be GRUB")
if boot.get("efiBootloaderId") != "zorix":
    fail("efiBootloaderId must be zorix")
if boot.get("installEFIFallback") is not True:
    fail("UEFI fallback boot files must remain enabled")

unpack = yaml.safe_load((MOD / "unpackfs.conf").read_text())
entries = unpack.get("unpack", [])
if not entries:
    fail("unpackfs.conf has no unpack entries")
exclusions = set(entries[0].get("exclude", []))
for required in ("/dev/*", "/proc/*", "/sys/*", "/run/*", "/tmp/*"):
    if required not in exclusions:
        fail(f"unpackfs exclusion missing: {required}")

services = yaml.safe_load((MOD / "services-systemd.conf").read_text())
units = {x.get("name"): x for x in services.get("units", []) if isinstance(x, dict)}
if not units.get("lightdm.service", {}).get("mandatory"):
    fail("lightdm.service must remain mandatory")
for unit in ("NetworkManager.service", "bluetooth.service", "fstrim.timer", "systemd-timesyncd.service"):
    if unit not in units:
        fail(f"required installed-system service missing: {unit}")

print("PASS: Zorix Calamares configuration invariants validated")

preuser_text = (MOD / "preuser.conf").read_text()
if "20-zorix-live.conf" not in preuser_text:
    fail("installed-system cleanup must remove the Live autologin configuration")
