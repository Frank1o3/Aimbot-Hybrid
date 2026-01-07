import time
import logging

import cv2
import numpy as np
from capture.capture import CaptureManager, BackendType

logging.basicConfig(
    format="%(asctime)s [%(levelname)s] %(message)s",
    level=logging.INFO,
    datefmt="%H:%M:%S",
)

win_name = "Opera"
logger = logging.getLogger(__name__)


def main() -> None:
    cm = CaptureManager(win_name)
    cm.set_backend(BackendType.Auto)  # Good: forces the working backend

    if not cm.init():
        logger.critical(f"Window '{win_name}' not found or initialization failed.")
        return

    logger.info(f"Successfully initialized capture:")
    logger.info(f"  Window size: {cm.buffer.width}x{cm.buffer.height}")
    logger.info(f"  Backend: {cm.get_backend_name()}")

    cm.start()
    logger.info("capture started. Press 'q' in the window to quit.")

    frame_count_at_start = cm.frame_count
    start_time = time.time()

    try:
        while True:
            # Get the latest frame as zero-copy NumPy array (H, W, 4) RGBA
            frame = cm.buffer.view

            # Convert to BGR for OpenCV display
            frame_bgr = cv2.cvtColor(frame, cv2.COLOR_RGBA2BGR)

            # Optional: overlay FPS
            current_time = time.time()
            elapsed = current_time - start_time
            if elapsed > 0:
                fps = (cm.frame_count - frame_count_at_start) / elapsed
            else:
                fps = 0.0

            cv2.putText(
                frame_bgr,
                f"FPS: {fps:.1f}  (Frames: {cm.frame_count})",
                (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX,
                1.0,
                (0, 255, 0),
                2,
                cv2.LINE_AA,
            )

            cv2.imshow("capture - Press 'q' to quit", frame_bgr)

            if cv2.waitKey(1) == ord("q"):
                break

    except KeyboardInterrupt:
        logger.info("capture interrupted by user.")
    except Exception as e:
        logger.exception(f"Unexpected error during capture: {e}")
    finally:
        final_time = time.time()
        total_time = final_time - start_time
        total_frames = cm.frame_count - frame_count_at_start

        if total_time > 0:
            avg_fps = total_frames / total_time
        else:
            avg_fps = 0.0

        logger.info(f"capture stopped.")
        logger.info(f"  Total frames captured: {total_frames}")
        logger.info(f"  Average FPS: {avg_fps:.1f}")

        cm.stop()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
