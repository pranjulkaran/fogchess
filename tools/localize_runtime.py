"""
@file tools/localize_runtime.py
@brief Localizes MinGW runtime DLLs into the build directory to prevent FileNotFoundError.
"""

import shutil
import os
import subprocess
from pathlib import Path

def localize():
    # Find where g++ lives on your system
    gxx_path = shutil.which("g++")
    if not gxx_path:
        print("[ERROR] Could not find G++ in your PATH. Is MinGW installed?")
        return

    mingw_bin_dir = Path(gxx_path).parent
    # This is where your library currently lives
    target_dir = Path("build")
    
    runtime_dlls = ["libwinpthread-1.dll", "libgcc_s_seh-1.dll", "libstdc++-6.dll"]
    
    print(f"[INFO] Auto-detected MinGW toolchain at: {mingw_bin_dir}")
    
    for dll in runtime_dlls:
        source_dll = mingw_bin_dir / dll
        dest_dll = target_dir / dll
        
        if source_dll.exists():
            shutil.copy2(source_dll, dest_dll)
            print(f"  -> Localized system dependency: {dll} -> {target_dir}")
        else:
            print(f"  [WARNING] Could not find {dll} in {mingw_bin_dir}")

if __name__ == "__main__":
    localize()