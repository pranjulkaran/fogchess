import ctypes
import os
import sys
import numpy as np

# --- 1. Path Injection for MinGW Dependencies ---
# This ensures Python can resolve libstdc++-6.dll and other background requirements.
mingw_bin = r"C:\mingw64\bin"
if sys.platform == "win32" and os.path.exists(mingw_bin):
    os.environ["PATH"] = mingw_bin + os.pathsep + os.environ["PATH"]

# --- 2. Locate Shared Library ---
base_dir = os.path.dirname(os.path.abspath(__file__))
possible_paths = [
    os.path.join(base_dir, "libfog_core.dll"),
    os.path.join(base_dir, "fog_core.dll"),
    os.path.join(base_dir, "build", "libfog_core.dll"),
    os.path.join(base_dir, "build", "fog_core.dll")
]

lib_path = next((p for p in possible_paths if os.path.exists(p)), None)
if not lib_path:
    raise FileNotFoundError(f"FogEngine Shared Library not found! Checked: {possible_paths}")

# --- 3. Load Library with winmode=0 for dependency resolution ---
if sys.platform == "win32" and sys.version_info >= (3, 8):
    _lib = ctypes.CDLL(lib_path, winmode=0)
else:
    _lib = ctypes.CDLL(lib_path)

# --- 4. Explicit C-ABI Signatures (Prevents Overflow Errors) ---
_lib.fog_engine_init.argtypes = [ctypes.c_uint32]
_lib.fog_engine_init.restype = ctypes.c_void_p

_lib.fog_engine_shutdown.argtypes = [ctypes.c_void_p]

_lib.fog_batch_step.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32]
_lib.fog_batch_step.restype = ctypes.c_int32

_lib.fog_batch_reset.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32]

_lib.fog_get_moves.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]

_lib.fog_get_layer3_tensor.argtypes = [
    ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint64, 
    ctypes.POINTER(ctypes.c_int32), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32), 
    ctypes.c_int32, ctypes.c_int32, ctypes.POINTER(ctypes.c_float)
]

_lib.fog_get_true_state_matrix.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_int8)]
_lib.fog_get_true_state_matrix.restype = ctypes.c_int32

_lib.fog_check_terminations.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
_lib.fog_check_terminations.restype = ctypes.c_int32

class FogEngine:
    def __init__(self, max_batches: int = 1024):
        self._handle = _lib.fog_engine_init(max_batches)
        if not self._handle:
            raise RuntimeError("Failed to initialize FogEngine.")

    def __del__(self):
        if hasattr(self, '_handle') and self._handle:
            _lib.fog_engine_shutdown(self._handle)

    def reset_batches(self, batch_indices: list[int]):
        count = len(batch_indices)
        c_indices = (ctypes.c_uint32 * count)(*batch_indices)
        _lib.fog_batch_reset(self._handle, c_indices, count)

    def step_batches(self, batch_indices: list[int], commands: list[int]):
        count = len(commands)
        c_indices = (ctypes.c_uint32 * len(batch_indices))(*batch_indices)
        c_commands = (ctypes.c_uint32 * count)(*commands)
        res = _lib.fog_batch_step(self._handle, c_indices, c_commands, count)
        if res != 0:
            raise RuntimeError(f"Engine failed to step batches. Code: {res}")

    def check_terminations(self, batch_index: int) -> int:
        c_mask = ctypes.c_uint32(0)
        res = _lib.fog_check_terminations(self._handle, batch_index, ctypes.byref(c_mask))
        if res != 0:
            raise RuntimeError("Engine failed to check terminations.")
        return c_mask.value

    def get_layer3_tensor(self, batch_index: int, lane: int, visible_mask: int, is_scout: bool = False) -> np.ndarray:
        tensor = np.zeros(14 * 8 * 8, dtype=np.float32)
        c_tensor_ptr = tensor.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
        c_scouted = (ctypes.c_int32 * 0)()
        
        _lib.fog_get_layer3_tensor(
            self._handle, batch_index, lane, visible_mask, 
            c_scouted, 0, c_scouted, 0, int(is_scout), c_tensor_ptr
        )
        return tensor.reshape((14, 8, 8))

    def get_true_state_matrix(self, batch_index: int, lane: int) -> np.ndarray:
        tensor = np.zeros(64, dtype=np.int8)
        c_tensor_ptr = tensor.ctypes.data_as(ctypes.POINTER(ctypes.c_int8))
        _lib.fog_get_true_state_matrix(self._handle, batch_index, lane, c_tensor_ptr)
        return tensor.reshape((8, 8)).astype(np.float32)