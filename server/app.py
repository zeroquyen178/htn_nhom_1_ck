from flask import Flask, request, jsonify, render_template

app = Flask(__name__)

# Cấu trúc dữ liệu lưu trữ trạng thái hệ thống
queue_system = {
    "last_ticket": 0,       # Số vé mới nhất phát ra từ Kiosk
    "total_waiting": 0,     # Số người đang xếp hàng
    "counters": {},         # Trạng thái hiện tại của từng quầy { "Counter_1": 5, ... }
    "admin_commands": []    # Hàng đợi lệnh "Next" chờ ESP32 lấy về
}

@app.route("/")
def home():
    """Hiển thị giao diện điều khiển (Dashboard)"""
    return render_template("index.html")

# --- CỔNG GIAO TIẾP VỚI ESP32 (Giao thức JSON) ---

@app.route("/queue", methods=["POST"])
def receive_queue():
    """ESP32 gọi vào đây để báo cáo trạng thái mới nhất"""
    data = request.json
    if not data:
        return jsonify({"status": "error"}), 400

    cid_str = data.get("counter_id") 
    q_num = data.get("queue_number")
    waiting = data.get("waiting")

    # Cập nhật thông tin từ ESP32 vào bộ nhớ Server
    queue_system["total_waiting"] = waiting
    if cid_str == "Kiosk_RFID":
        queue_system["last_ticket"] = q_num
    else:
        queue_system["counters"][cid_str] = q_num
    
    print(f"[ESP32 REPORT] {cid_str} -> Ticket: {q_num}, Waiting: {waiting}")
    return jsonify({"status": "success"})

@app.route("/get_next", methods=["GET"])
def get_next():
    """ESP32 polling vào đây mỗi 1.5s để lấy lệnh gọi số"""
    if queue_system["admin_commands"]:
        cmd = queue_system["admin_commands"].pop(0)
        print(f"[SYSTEM] Sending NEXT command to ESP32: {cmd}")
        return jsonify(cmd), 200
    
    # Nếu không có lệnh nào, trả về 204 No Content
    return jsonify({"status": "no_command"}), 204

# --- CỔNG GIAO TIẾP VỚI GIAO DIỆN WEB ---

@app.route("/dashboard", methods=["GET"])
def get_dashboard():
    """API để giao diện Web cập nhật số liệu hiển thị"""
    return jsonify(queue_system)

@app.route("/control", methods=["POST"])
def control_system():
    """Xử lý khi Admin nhấn nút 'Next' trên Web"""
    data = request.json
    counter_id = data.get("counter_id") 
    action = data.get("action")

    if action == "next" and counter_id is not None:
        # Thêm lệnh vào hàng đợi để ESP32 lấy về qua /get_next
        queue_system["admin_commands"].append({
            "action": "next",
            "counter_id": int(counter_id) # Chuyển sang kiểu int để ESP32 dùng cJSON_GetObjectItem
        })
        print(f"[ADMIN] Next command queued for Counter: {counter_id}")
        return jsonify({"status": "next_queued"})

    return jsonify({"status": "unknown_action"}), 400

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)