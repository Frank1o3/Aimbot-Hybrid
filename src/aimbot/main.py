import xwayland_capture

# Access WaylandCapture directly
capture = xwayland_capture.XWaylandCapture()
capture.setup(1920, 1080)
frame = capture.capture_frame()
print(f"Captured frame size: {len(frame)} bytes")
