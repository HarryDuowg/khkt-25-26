import cv2
import numpy as np
import serial
import time
from ultralytics import YOLO

# =========================
# SERIAL
# =========================
ser = serial.Serial('COM3', 9600, timeout=1)
time.sleep(2)
print("✅ Arduino connected")

# =========================
# YOLO
# =========================
model = YOLO(r"E:\KHKT - GIAO THONG\best.pt")
names = model.names

cap = cv2.VideoCapture(1)
cap.set(3, 640)
cap.set(4, 480)

# =========================
# LANE CONFIG
# =========================
LANES = {
    "L1": np.array([[220, 0],   [320, 0],   [320, 170], [220, 170]]),
    "L2": np.array([[320, 310], [420, 310], [420, 480], [320, 480]]),
    "L3": np.array([[0, 240],   [220, 240], [220, 310], [0, 310]]),
    "L4": np.array([[420, 170], [640, 170], [640, 240], [420, 240]])
}

INTERSECTION = np.array([[220,170],[420,170],[420,310],[220,310]])

def in_poly(pt, poly):
    return cv2.pointPolygonTest(poly, pt, False) >= 0

PRIORITY_CLASSES = ["fire trucks", "police car"]

# =========================
# MAIN LOOP
# =========================
while True:
    ret, frame = cap.read()
    if not ret:
        break

    results = model(frame, conf=0.5, verbose=False)[0]

    lane_count = {k: 0 for k in LANES}
    priority_lane = "0"

    for box in results.boxes:
        cls_id = int(box.cls[0])
        cls_name = names[cls_id].lower()

        x1,y1,x2,y2 = map(int, box.xyxy[0])
        cx, cy = (x1+x2)//2, (y1+y2)//2

        # ===== XE ƯU TIÊN =====
        if cls_name in PRIORITY_CLASSES:
            for lane, poly in LANES.items():
                if in_poly((cx, cy), poly):
                    priority_lane = lane[-1]
                    break

        # bỏ đếm trong giao lộ
        if in_poly((cx, cy), INTERSECTION):
            continue

        for lane, poly in LANES.items():
            if in_poly((cx, cy), poly):
                lane_count[lane] += 1
                break

        # ===== VẼ BOX =====
        color = (0,0,255) if cls_name in PRIORITY_CLASSES else (0,255,0)
        cv2.rectangle(frame,(x1,y1),(x2,y2),color,2)
        cv2.putText(frame, cls_name, (x1,y1-5),
                    cv2.FONT_HERSHEY_SIMPLEX,0.6,color,2)

    # =========================
    # SEND SERIAL
    # =========================
    if ser.in_waiting:
        cmd = ser.readline().decode().strip()
        if cmd == "READY":
            data = (
                f"L1:{lane_count['L1']},"
                f"L2:{lane_count['L2']},"
                f"L3:{lane_count['L3']},"
                f"L4:{lane_count['L4']},"
                f"P:{priority_lane}\n"
            )
            ser.write(data.encode())
            print("➡️ GỬI:", data.strip())

    # =========================
    # UI
    # =========================
    for lane, poly in LANES.items():
        cv2.polylines(frame,[poly],True,(255,255,255),2)
        x,y = poly[0]
        cv2.putText(frame,f"{lane}:{lane_count[lane]}",
                    (x+5,y+25),cv2.FONT_HERSHEY_SIMPLEX,
                    0.7,(0,255,0),2)

    cv2.polylines(frame,[INTERSECTION],True,(0,255,255),2)

    if priority_lane != "0":
        cv2.putText(frame,f"UU TIEN: L{priority_lane}",
                    (10,30),cv2.FONT_HERSHEY_SIMPLEX,
                    0.9,(0,0,255),2)

    cv2.imshow("AI PHAN LUONG GIAO THONG", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
ser.close()