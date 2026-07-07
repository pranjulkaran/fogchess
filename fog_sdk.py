import ctypes
import os
import sys
import numpy as np

# Adjust based on your OS and build output folder
base_dir = os.path.dirname(__file__)
lib_name = "libfog_core.so" if sys.platform != "win32" else "libfog_core.dll"
lib_path = os.path.join(base_dir, "build", lib_name)

if not os.path.exists(lib_path):
    raise FileNotFoundError(f"Shared library not found at {lib_path}. Did you run CMake?")

_lib = ctypes.CDLL(lib_path)

# C-ABI Signatures
_lib.fog_engine_init.argtypes = [ctypes.c_uint32]
_lib.fog_engine_init.restype = ctypes.c_void_p
_lib.fog_engine_shutdown.argtypes = [ctypes.c_void_p]
_lib.fog_batch_reset.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32]
_lib.fog_get_moves.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
_lib.fog_get_layer3_tensor.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint64, ctypes.POINTER(ctypes.c_int32), ctypes.c_int32, ctypes.POINTER(ctypes.c_int32), ctypes.c_int32, ctypes.c_int32, ctypes.POINTER(ctypes.c_float)]

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

    def get_legal_moves(self, batch_index: int, lane: int) -> list[int]:
        c_commands = (ctypes.c_uint32 * 256)() 
        c_count = ctypes.c_uint32(0)
        _lib.fog_get_moves(self._handle, batch_index, lane, c_commands, ctypes.byref(c_count))
        return [c_commands[i] for i in range(c_count.value)]

    def get_layer3_tensor(self, batch_index: int, lane: int, visible_mask: int, is_scout: bool = False) -> np.ndarray:
        # Pre-allocate numpy array and map it directly to a C-pointer (Zero-Copy)
        tensor = np.zeros(14 * 8 * 8, dtype=np.float32)
        c_tensor_ptr = tensor.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
        
        c_scouted = (ctypes.c_int32 * 0)() # Placeholder for empty arrays
        
        _lib.fog_get_layer3_tensor(
            self._handle, batch_index, lane, visible_mask, 
            c_scouted, 0, c_scouted, 0, int(is_scout), c_tensor_ptr
        )
        
        return tensor.reshape((14, 8, 8))