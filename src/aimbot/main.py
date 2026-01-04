from capture.capture import CaptureManager, BackendType
import time

cap = CaptureManager("Sober", BackendType.Auto)

if cap.init():
    print(f"Backend: {cap.get_backend_name()}")
    cap.start()
    time.sleep(0.5)
    initial_count = cap.frame_count
    print(f"Initial frame count: {initial_count}")

    if initial_count == 0:
        print("✗ No frames captured!")
        cap.stop()

    print("\n--- Buffer Access Test ---")
    try:
        buffer = cap.buffer
        print("✓ Got buffer object")
        print(f"  Width: {buffer.width}")
        print(f"  Height: {buffer.height}")
        print(f"  Stride: {buffer.stride}")

        # Get numpy view
        frame = buffer.view
        print("✓ Got numpy view")
        print(f"  Shape: {frame.shape}")
        print(f"  Dtype: {frame.dtype}")
        print(f"  Memory: {frame.nbytes} bytes ({frame.nbytes / 1024 / 1024:.2f} MB)")

        # Check if we're getting new frames
        time.sleep(0.2)
        new_count = cap.frame_count
        frames_captured = new_count - initial_count
        print(f"\n✓ Captured {frames_captured} frames in 0.2 seconds")
        print(f"  Frame rate: ~{frames_captured / 0.2:.1f} fps")

    except Exception as e:
        print(f"✗ Buffer access failed: {e}")
        import traceback

        traceback.print_exc()
        cap.stop()
