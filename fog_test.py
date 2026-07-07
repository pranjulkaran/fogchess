from fog_sdk import FogEngine

if __name__ == "__main__":
    print("[Python] Booting Deterministic SIMD Kernel...")
    engine = FogEngine(max_batches=10)

    print("[Python] Resetting Batch 0...")
    engine.reset_batches([0])

    moves = engine.get_legal_moves(batch_index=0, lane=0)
    print(f"[Python] Extracted {len(moves)} valid moves. First 5 packed commands: {moves[:5]}")

    print("[Python] Extracting Zero-Copy Observation Tensor...")
    # 0xFFFFFFFFFFFFFFFF is full board visibility
    obs_tensor = engine.get_layer3_tensor(batch_index=0, lane=0, visible_mask=0xFFFFFFFFFFFFFFFF)
    
    print(f"[Python] Tensor Shape: {obs_tensor.shape}")
    print("[Python] All systems operational.")