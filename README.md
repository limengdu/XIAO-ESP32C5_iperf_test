# ESP32-C5 WiFi 吞吐量测试工具

基于 ESP-IDF 的 WiFi 性能测试工具，使用行业标准 iperf 协议测量 ESP32-C5 的无线网络吞吐量。

## 功能特性

- 支持 WiFi 6 (802.11ax) 协议
- 支持 2.4GHz 和 5GHz 双频段
- TCP/UDP 双向吞吐量测试
- 实时 WiFi 统计信息显示
- 交互式命令行界面

---

## 快速开始

### 1. 环境准备

确保已安装 ESP-IDF v5.5 或更高版本。

### 2. 编译与烧录

```bash
# 进入项目目录
cd esp32c5_iperf_test

# 设置目标芯片
idf.py --preview set-target esp32c5

# 编译
idf.py build

# 烧录并打开串口监视器
idf.py flash monitor
```

> **Xiao ESP32C5 用户注意**：本项目已配置使用 USB Serial JTAG 作为控制台，直接通过 USB-C 连接即可。

### 3. 连接 WiFi

烧录成功后，终端会显示 `iperf>` 提示符。

**扫描可用网络（可选）：**
```
scan
```

**连接 WiFi：**
```
sta 你的WiFi名称 你的WiFi密码
```

连接成功后会显示获取到的 IP 地址，**请记录此 IP**。

---

## WiFi 测速指南

### 准备工作

1. **电脑安装 iperf**
   ```bash
   # macOS
   brew install iperf
   
   # Ubuntu/Debian
   sudo apt install iperf
   
   # Windows
   # 下载: https://iperf.fr/iperf-download.php
   ```

2. **确保电脑和 ESP32-C5 连接同一 WiFi 网络**

3. **查看电脑 IP 地址**
   ```bash
   # macOS/Linux
   ifconfig | grep "inet " | grep -v 127.0.0.1
   
   # Windows
   ipconfig
   ```

---

### TCP 测速

#### 下载测试（ESP32 接收数据）

**ESP32 终端：**
```
iperf -s -i 1
```

**电脑终端：**
```bash
iperf -c <ESP32的IP地址> -i 1 -t 60
```

#### 上传测试（ESP32 发送数据）

**电脑终端：**
```bash
iperf -s -i 1
```

**ESP32 终端：**
```
iperf -c <电脑的IP地址> -i 1 -t 60
```

---

### UDP 测速

#### 下载测试

**ESP32 终端：**
```
iperf -s -u -i 1
```

**电脑终端：**
```bash
iperf -c <ESP32的IP地址> -u -b 100M -i 1 -t 60
```

#### 上传测试

**电脑终端：**
```bash
iperf -s -u -i 1
```

**ESP32 终端：**
```
iperf -c <电脑的IP地址> -u -b 100M -i 1 -t 60
```

---

### 常用命令参考

| 命令 | 说明 |
|------|------|
| `help` | 显示所有可用命令 |
| `scan` | 扫描 WiFi 网络 |
| `sta SSID PASSWORD` | 连接 WiFi |
| `sta -d` | 断开 WiFi |
| `iperf -s` | 启动 TCP 服务器 |
| `iperf -s -u` | 启动 UDP 服务器 |
| `iperf -c IP` | TCP 客户端模式 |
| `iperf -c IP -u` | UDP 客户端模式 |
| `iperf -a` | 停止测试 |
| `ping IP` | Ping 连通性测试 |

---

## 产品性能测试规范

以下测试方法符合行业规范，测试结果可用于产品宣传材料。

### 测试环境要求

| 项目 | 要求 |
|------|------|
| 测试场地 | 开阔室内环境，无明显遮挡物 |
| 路由器距离 | 1米（近场）/ 5米（中场）/ 10米（远场） |
| 路由器规格 | 支持 WiFi 6 (802.11ax)，千兆有线端口 |
| 测试电脑 | 有线连接路由器（排除无线干扰） |
| 信道环境 | 使用 WiFi 分析工具确认信道无严重干扰 |
| 测试次数 | 每项测试至少 3 次，取平均值 |
| 单次时长 | 60 秒 |

### 标准测试流程

#### 1. 环境准备
```bash
# 确认路由器设置
- WiFi 6 模式开启
- 频宽设置：2.4GHz-40MHz / 5GHz-80MHz
- 记录信道号
```

#### 2. 2.4GHz 频段测试

**连接 2.4GHz 网络：**
```
sta YourSSID_2.4G Password
```

**TCP 吞吐量测试：**
```bash
# 电脑端（有线连接路由器）
iperf -s -i 3

# ESP32 端
iperf -c <电脑IP> -i 3 -t 60
```

**记录数据格式：**
```
测试项目：TCP 上传
频段：2.4GHz
信道：6
距离：1米
结果：XX.X Mbps（3次平均值）
```

#### 3. 5GHz 频段测试

**连接 5GHz 网络：**
```
sta YourSSID_5G Password
```

重复上述 TCP/UDP 测试。

#### 4. 数据记录模板

| 测试项 | 2.4GHz (1m) | 2.4GHz (5m) | 5GHz (1m) | 5GHz (5m) |
|--------|-------------|-------------|-----------|-----------|
| TCP 上传 | - Mbps | - Mbps | - Mbps | - Mbps |
| TCP 下载 | - Mbps | - Mbps | - Mbps | - Mbps |
| UDP 上传 | - Mbps | - Mbps | - Mbps | - Mbps |
| UDP 下载 | - Mbps | - Mbps | - Mbps | - Mbps |

### 宣传材料合规建议

1. **标注测试条件**
   > WiFi 吞吐量最高可达 XX Mbps*
   > *测试条件：5GHz 频段，802.11ax 模式，1米距离，理想信道环境

2. **使用"最高可达"而非绝对值**
   - ✅ "WiFi 吞吐量最高可达 50Mbps"
   - ❌ "WiFi 速度 50Mbps"

3. **区分理论值与实测值**
   - 理论值：协议规定的最大速率
   - 实测值：iperf 测得的实际吞吐量

4. **提供完整测试报告**
   - 测试日期
   - 固件版本
   - 测试设备型号
   - 测试环境描述
   - 多次测试的原始数据

---

## 典型性能参考

以下数据仅供参考，实际性能因环境而异：

| 场景 | TCP 吞吐量 | UDP 吞吐量 |
|------|-----------|-----------|
| 5GHz 近场 (1m) | 40-60 Mbps | 50-80 Mbps |
| 5GHz 中场 (5m) | 30-50 Mbps | 40-60 Mbps |
| 2.4GHz 近场 (1m) | 25-40 Mbps | 30-50 Mbps |
| 2.4GHz 中场 (5m) | 15-30 Mbps | 20-40 Mbps |

> ESP32-C5 支持 WiFi 6，在兼容路由器下可获得更佳性能。

---

## 故障排除

### 终端无法输入
- 确认使用 `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` 配置
- 重新执行 `idf.py set-target esp32c5` 并重新编译

### WiFi 连接失败
- 检查 SSID 和密码是否正确
- 尝试使用 `scan` 命令确认网络可见
- 确认路由器未开启 MAC 地址过滤

### iperf 测试无数据
- 确认 ESP32 和电脑在同一网络
- 检查防火墙是否阻止 iperf（默认端口 5001）
- 尝试 `ping` 命令测试连通性

---

## 许可证

本项目代码基于 Public Domain / CC0 协议开源。
