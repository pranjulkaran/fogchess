import ctypes
import os
import sys

# --- Force-load MinGW dependencies ---
# This ensures that the GCC runtime (libstdc++, libgcc, etc.) 
# is loaded into memory before our kernel tries to link to it.
mingw_bin = r"C:\mingw64\bin"
if os.path.exists(mingw_bin):
    # Prepend the bin directory to the system PATH so the DLL loader finds them
    os.environ["PATH"] = mingw_bin + os.pathsep + os.environ["PATH"]
    
    # Explicitly load the runtime dependencies
    try:
        ctypes.CDLL("libstdc++-6.dll")
        ctypes.CDLL("libgcc_s_seh-1.dll")
        ctypes.CDLL("libwinpthread-1.dll")
    except OSError as e:
        print(f"Warning: Could not pre-load MinGW dependencies: {e}")

# --- Load the Shared Library ---
base_dir = os.path.dirname(__file__)
lib_path = os.path.join(base_dir, "build", "libfog_core.dll")

if not os.path.exists(lib_path):
    raise FileNotFoundError(f"Library not found at {lib_path}")

_lib = ctypes.CDLL(lib_path)

# ... (rest of your _lib signatures remain exactly the same)

# --- Define C-ABI Signatures ---
_lib.fog_engine_init.argtypes = [ctypes.c_uint32]
_lib.fog_engine_init.restype = ctypes.c_void_p

_lib.fog_engine_shutdown.argtypes = [ctypes.c_void_p]
_lib.fog_engine_shutdown.restype = None

_lib.fog_batch_step.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32]
_lib.fog_batch_step.restype = ctypes.c_int32

_lib.fog_batch_reset.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32]
_lib.fog_batch_reset.restype = ctypes.c_int32

_lib.fog_get_moves.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
_lib.fog_get_moves.restype = ctypes.c_int32

_lib.fog_check_terminations.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
_lib.fog_check_terminations.restype = ctypes.c_int32


# --- Pythonic Wrapper Class ---
class FogEngine:
    """Pythonic interface for the high-performance FogEngine C++ kernel."""
    
    def __init__(self, max_batches: int = 1024):
        self._handle = _lib.fog_engine_init(max_batches)
        if not self._handle:
            raise RuntimeError("Failed to initialize FogEngine. Out of memory?")

    def __del__(self):
        if hasattr(self, '_handle') and self._handle:
            _lib.fog_engine_shutdown(self._handle)

    def reset_batches(self, batch_indices: list[int]):
        """Resets the specified batches (4 parallel games per batch) to starting positions."""
        count = len(batch_indices)
        c_indices = (ctypes.c_uint32 * count)(*batch_indices)
        res = _lib.fog_batch_reset(self._handle, c_indices, count)
        if res != 0: raise RuntimeError(f"Engine failed to reset batches. Code: {res}")

    def get_legal_moves(self, batch_index: int, lane: int) -> list[int]:
        """Returns a list of packed 32-bit integer commands representing legal moves."""
        c_commands = (ctypes.c_uint32 * 256)() # Allocate max move buffer
        c_count = ctypes.c_uint32(0)
        
        res = _lib.fog_get_moves(self._handle, batch_index, lane, c_commands, ctypes.byref(c_count))
        if res != 0: raise RuntimeError(f"Engine failed to generate moves. Code: {res}")
        
        return [c_commands[i] for i in range(c_count.value)]