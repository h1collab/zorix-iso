#!/usr/bin/env python3
import argparse, os, pathlib, struct, shutil, math

MAGIC=b"ZORIXPK1"
ENTRY=struct.Struct("<120sQQII")
HDR=struct.Struct("<8sII")

def write_text(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")

def ppm(path, phase, accent):
    w,h=1280,800
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as f:
        f.write(f"P6\n{w} {h}\n255\n".encode())
        for y in range(h):
            row=bytearray()
            ny=y/(h-1)
            for x in range(w):
                nx=x/(w-1)
                base=(1.0-ny)
                r=int(6+10*base+accent[0]*0.10)
                g=int(15+24*base+accent[1]*0.10)
                b=int(28+46*base+accent[2]*0.12)
                wave=math.sin(nx*7.0+phase+ny*2.6)
                glow=max(0.0,1.0-((nx-(0.22+0.55*ny))**2*4.5+(ny-0.48)**2*2.2))
                glow2=max(0.0,1.0-((nx-(0.78-0.30*ny))**2*5.0+(ny-0.34)**2*3.0))
                r=min(255,max(0,int(r+glow*accent[0]*0.46+glow2*accent[2]*0.13+wave*3)))
                g=min(255,max(0,int(g+glow*accent[1]*0.38+glow2*accent[0]*0.10+wave*4)))
                b=min(255,max(0,int(b+glow*accent[2]*0.36+glow2*accent[1]*0.26+wave*6)))
                row += bytes((r,g,b))
            f.write(row)

def stage(root, repo):
    if root.exists():
        shutil.rmtree(root)
    root.mkdir(parents=True)
    write_text(root/"System/manifest.txt",
        "Zorix Native System 0.7\nKernel=independent\nABI=0.7\nDesktop=Glass\nRootFS=ZPK1\n")
    write_text(root/"System/Browser/start.txt",
        "ZORIX START\nNative Browser resources are mounted from the Zorix root filesystem.\n")
    write_text(root/"System/Browser/bookmarks.txt",
        "Zorix Home|zorix://start\nSDK Docs|zorix://sdk\nStore|zorix://store\n")
    write_text(root/"System/Store/catalog.txt",
        "terminal|Terminal|1.0|system\nfiles|Files|1.0|system\nsettings|Settings|1.0|system\n"
        "browser|Zorix Browser|0.7|system\nsdk|Zorix SDK|0.7|developer\n")
    write_text(root/"System/Recovery/README.txt",
        "Zorix Recovery Environment\nUse the UEFI boot image and rootfs payload to repair a native installation.\n")
    write_text(root/"System/Installer/install.manifest",
        "schema=1\nboot=EFI/BOOT/BOOTX64.EFI\nrootfs=System/rootfs.zpk\nfilesystem=zorixfs\n")
    write_text(root/"System/Apps/Terminal/app.manifest","id=terminal\nname=Terminal\nabi=0.7\n")
    write_text(root/"System/Apps/Files/app.manifest","id=files\nname=Files\nabi=0.7\n")
    write_text(root/"System/Apps/Settings/app.manifest","id=settings\nname=Settings\nabi=0.7\n")
    write_text(root/"System/Apps/Browser/app.manifest","id=browser\nname=Zorix Browser\nabi=0.7\n")
    write_text(root/"System/Apps/Store/app.manifest","id=store\nname=Zorix Store\nabi=0.7\n")

    sdk_src=repo/"sdk"
    if sdk_src.exists():
        shutil.copytree(sdk_src, root/"Developer/SDK", dirs_exist_ok=True)
    legacy=repo/"release/1.3.0/ui"
    if legacy.exists():
        shutil.copytree(legacy, root/"System/Migration/Glass-1.3", dirs_exist_ok=True)

    palettes=[
        (44,142,255),(111,86,255),(23,199,174),(255,105,137),
        (73,172,255),(116,225,175),(161,118,255),(255,174,85),
        (46,118,188),(82,173,142),(104,87,181),(189,78,117),
        (72,198,255),(138,96,255),(34,220,184),(255,122,153),
        (58,160,230),(104,211,164),(181,132,255),(255,192,103),
        (56,132,202),(96,184,154),(126,108,198),(207,94,134)
    ]
    for i,pal in enumerate(palettes):
        ppm(root/f"System/Assets/Wallpapers/tide-{i+1:02d}.ppm",i*0.53,pal)

def build(root, out):
    files=[]
    for p in sorted(root.rglob("*")):
        if p.is_file():
            rel="/"+p.relative_to(root).as_posix()
            data=p.read_bytes()
            files.append((rel,data))
    index_bytes=HDR.size+len(files)*ENTRY.size
    offset=index_bytes
    entries=[]
    for rel,data in files:
        raw=rel.encode("utf-8")
        if len(raw)>=120:
            raise SystemExit(f"path too long: {rel}")
        entries.append((raw+b"\0"*(120-len(raw)),offset,len(data),1,0))
        offset+=len(data)
    out.parent.mkdir(parents=True,exist_ok=True)
    with out.open("wb") as f:
        f.write(HDR.pack(MAGIC,1,len(files)))
        for e in entries:f.write(ENTRY.pack(*e))
        for _,data in files:f.write(data)
    print(f"ZPK files={len(files)} bytes={out.stat().st_size}")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--repo",default=".")
    ap.add_argument("--stage",required=True)
    ap.add_argument("--out",required=True)
    a=ap.parse_args()
    repo=pathlib.Path(a.repo).resolve()
    root=pathlib.Path(a.stage).resolve()
    stage(root,repo)
    build(root,pathlib.Path(a.out).resolve())

if __name__=="__main__":main()
