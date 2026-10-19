import paho.mqtt.client as mqtt
import json
import csv
import os
from datetime import datetime

# ================= 配置参数 =================
BROKER = "broker.emqx.io"
PORT = 1883
TOPIC = "/esp32/cym/sensor/light"
CSV_FILE = "sensor_data.csv"

# 初始化 CSV 文件表头（如果文件不存在的话）
if not os.path.exists(CSV_FILE):
    with open(CSV_FILE, mode='w', newline='', encoding='utf-8') as file:
        writer = csv.writer(file)
        writer.writerow(["Timestamp", "Light_Value"])


# ================= 回调函数 =================

# 相当于你 ESP32 里的 MQTT_EVENT_CONNECTED
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print(f"✅ 成功连接到 MQTT 服务器: {BROKER}")
        # 连上之后立刻订阅主题
        client.subscribe(TOPIC)
        print(f"📡 已成功订阅主题: {TOPIC}\n等待接收数据...\n" + "-" * 30)
    else:
        print(f"❌ 连接失败，返回码: {reason_code}")


# 相当于你 ESP32 里的 MQTT_EVENT_DATA
def on_message(client, userdata, msg):
    try:
        # 1. 解码收到的原始数据
        payload_str = msg.payload.decode('utf-8')

        # 2. 将 JSON 字符串解析为 Python 字典
        data = json.loads(payload_str)
        light_val = data.get("light_value")

        # 3. 获取当前精准时间
        current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # 4. 在终端打印炫酷的日志
        print(f"[{current_time}] 收到数据 -> 光照强度: {light_val}")

        # 5. 落地！将数据追加写入 CSV 文件
        with open(CSV_FILE, mode='a', newline='', encoding='utf-8') as file:
            writer = csv.writer(file)
            writer.writerow([current_time, light_val])

    except json.JSONDecodeError:
        print(f"⚠️ 数据解析失败，收到的不是标准 JSON: {msg.payload}")
    except Exception as e:
        print(f"❌ 发生未知错误: {e}")


# ================= 主程序 =================
if __name__ == "__main__":
    # 创建 MQTT 客户端实例 (使用最新的 API 版本 callback_api_version=2)
    client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)

    # 绑定回调函数（这不就是你用过的函数指针嘛！）
    client.on_connect = on_connect
    client.on_message = on_message

    print("正在尝试连接云端服务器...")
    # 发起连接
    client.connect(BROKER, PORT, 60)

    # 启动网络循环（相当于开启了一个后台 Task 死循环，专门监听网络数据）
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n🛑 用户手动终止程序，退出监听。")
        client.disconnect()