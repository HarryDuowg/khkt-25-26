import cv2
import numpy as np
import serial
import time
from ultralytics import YOLO

# =========================
# KẾT NỐI SERIAL
# =========================
try:
    # Timeout thấp để vòng lặp không bị khựng khi đọc Serial
    ser = serial.Serial('COM4', 9600, timeout=0.01) 
    time.sleep(2)
    ser.flushInput()
    print("✅ Kết nối Arduino thành công")
except Exception as e:
    print(f"❌ Lỗi kết nối: {e}")
    exit()

# =========================
# CẤU HÌNH YOLO & CAMERA
# =========================
model = YOLO(r"E:\KHKT - GIAO THONG\3\best.pt")
names = model.names

cap = cv2.VideoCapture(1)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

# Tọa độ Lane (Giữ nguyên của bạn)
LANES = {
    "L1": np.array([[278, 24], [343, 39], [309, 170], [244, 152]]),
    "L2": np.array([[263, 334], [321, 346], [287, 481], [227, 474]]),
    "L3": np.array([[61, 193], [202, 226], [186, 287], [46, 253]]),
    "L4": np.array([[385, 213], [509, 248], [496, 303], [372, 272]]),
}
INTERSECTION = np.array([[237, 174], [361, 209], [334, 325], [205, 293]])
PRIORITY_CLASSES = ["fire trucks", "police car"]

def in_poly(pt, poly):
    return cv2.pointPolygonTest(poly, pt, False) >= 0

# Biến kiểm soát luồng dữ liệu
waiting_for_ready = True # Mặc định bắt đầu sẽ đợi ESP chạy xong safe mode và gọi READY
prio_sent_this_cycle = False   # Lưu vết đã gửi xe ưu tiên trong chu kỳ chưa
empty_sent_this_cycle = False  # Lưu vết đã gửi làn trống trong chu kỳ chưa

while True:
    ret, frame = cap.read()
    if not ret: break

    results = model(frame, conf=0.5, verbose=False)[0]
    lane_count = {k: 0 for k in LANES}
    priority_lane = "0"

    # --- VẼ LÀN ĐƯỜNG CỐ ĐỊNH ---
    for lane, poly in LANES.items():
        cv2.polylines(frame, [poly], True, (255, 255, 0), 2)
    cv2.polylines(frame, [INTERSECTION], True, (0, 255, 255), 2)

    # --- XỬ LÝ NHẬN DIỆN ---
    for box in results.boxes:
        cls_id = int(box.cls[0])
        cls_name = names[cls_id].lower()
        x1, y1, x2, y2 = map(int, box.xyxy[0])
        cx, cy = (x1 + x2) // 2, (y1 + y2) // 2

        is_priority = False
        if cls_name in PRIORITY_CLASSES:
            is_priority = True
            for lane, poly in LANES.items():
                if in_poly((cx, cy), poly):
                    priority_lane = lane[-1]
                    break

        if not in_poly((cx, cy), INTERSECTION):
            for lane, poly in LANES.items():
                if in_poly((cx, cy), poly):
                    lane_count[lane] += 1
                    break

        # Vẽ Box
        color = (0, 0, 255) if is_priority else (0, 255, 0)
        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
        cv2.putText(frame, cls_name, (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)

    # --- HIỂN THỊ SỐ LƯỢNG XE ---
    for lane, poly in LANES.items():
        cv2.putText(frame, f"{lane}: {lane_count[lane]}", (poly[0][0], poly[0][1] + 25), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

    # --- LOGIC GỬI SERIAL ---
    l12 = lane_count["L1"] + lane_count["L2"]
    l34 = lane_count["L3"] + lane_count["L4"]
    data_str = f"L1:{lane_count['L1']},L2:{lane_count['L2']},L3:{lane_count['L3']},L4:{lane_count['L4']},P:{priority_lane}\n"
    
    # Phân tách Điều kiện đặc biệt
    prio_special = (priority_lane != "0")
    empty_special = (l12 > 0 and l34 == 0) or (l34 > 0 and l12 == 0)

    # 1. Đọc tín hiệu từ Arduino (đọc sạch buffer để tránh đọng)
    while ser.in_waiting > 0:
        try:
            line = ser.readline().decode('utf-8').strip()
            if len(line) > 0:
                if "READY" in line:
                    waiting_for_ready = False
                    prio_sent_this_cycle = False   # Mở khóa báo ưu tiên cho chu kỳ mới
                    empty_sent_this_cycle = False  # Mở khóa báo làn trống cho chu kỳ mới
                    print("🔄 Arduino READY - Bắt đầu chu kỳ mới")
                else:
                    print(f"[{line}]")  # In tất cả các log khác để debug
        except: pass

    # 2. Xử lý Gửi dữ liệu khẩn cấp (Mỗi sự kiện khẩn cấp chỉ gửi đúng 1 lần duy nhất trong chu kỳ)
    send_emergency = False
    if prio_special and not prio_sent_this_cycle:
        send_emergency = True
        prio_sent_this_cycle = True
    if empty_special and not empty_sent_this_cycle:
        send_emergency = True
        empty_sent_this_cycle = True

    if send_emergency:
        ser.write(data_str.encode())
        print(f"🚨 CẬP NHẬT KHẨN CẤP: {data_str.strip()}")
        waiting_for_ready = True # Khóa gửi dữ liệu chu kỳ lại, đợi READY tiếp theo

    # 3. Gửi dữ liệu Chu kỳ (Chỉ Gửi khi nhận được READY)
    # Gửi cả khi L1-4 đều = 0 (tránh đè dữ liệu vì chỉ gửi 1 lần)
    if not waiting_for_ready:
        ser.write(data_str.encode())
        print(f"✅ GỬI DATA CHU KỲ: {data_str.strip()}")
        waiting_for_ready = True

    # Hiển thị cảnh báo ưu tiên lên màn hình
    if priority_lane != "0":
        cv2.putText(frame, f"UU TIEN: LAN {priority_lane}", (200, 50), 
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 3)

    cv2.imshow("AI TRAFFIC MANAGEMENT", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'): break

cap.release()
cv2.destroyAllWindows()
ser.close()
