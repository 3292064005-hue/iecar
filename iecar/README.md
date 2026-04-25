# 2021-F drug_car PlatformIO 工程

本仓库包含电赛小车三端代码：STM32F407 主控、K210 识别端、OpenMV 巡线端。

## 目录

| 路径 | 说明 |
|---|---|
| `stm32f407_platformio/` | STM32F407 PlatformIO 工程，含 `car1` / `car2` / `native` 环境 |
| `k210代码/` | K210 目标识别脚本、模型和标签，运行时导入生成的 `protocol_contract.py` |
| `opemv代码/` | OpenMV 巡线脚本，运行时导入生成的 `protocol_contract.py` |
| `tools/` | 协议生成、协议回放、黄金帧验证、静态检查脚本 |
| `docs/` | 协议规格、维护地图、迁移说明 |

## 构建与验证

```bash
./tools/check_static.sh quick     # Python 协议/状态回放 + 生成物同步 + wrapper 边界 + host C 测试
./tools/check_static.sh strict    # quick + pio run -e car1 + pio run -e car2 + pio test -e native
```

`strict` 模式要求主机安装 PlatformIO CLI。当前脚本不会把未执行的 PlatformIO 构建伪装成已验证结果。

## 协议维护规则

只改 `tools/protocol_contract.py`，然后执行：

```bash
./tools/generate_protocol_contract.py
```

生成物包括 STM32 C 头文件、host 测试头文件、K210/OpenMV Python 常量文件。`quick` 会检查生成物是否与单源配置同步。

## 主链路入口

- `stm32f407_platformio/platformio.ini`
- `stm32f407_platformio/src/car1/USER/main.c`
- `stm32f407_platformio/src/car2/USER/main.c`
- `stm32f407_platformio/src/COMMON/CHE/car_project_config_shared.h`
- `stm32f407_platformio/src/COMMON/CHE/protocol_core_shared.h`
- `docs/protocol_spec.md`

## 本轮架构落地重点

1. Task2 正常完成与安全停车语义分离。
2. 策略层非法 `detect` 防护，避免未定义移位。
3. OpenMV 首帧闸门与 K210 可靠性门控补齐。
4. K210/OpenMV wire frame 增加 version byte，并明确区分 wire frame 与 STM32 RX buffer。
5. CarLink 增加版本校验、角色矩阵、重复包幂等、兼容 sender role 归一化和诊断计数。
6. 增加结构化诊断 ring buffer 与高频故障节流。
7. 增加 quick/strict 两级验证入口、host C 测试和 PlatformIO native 测试入口。
