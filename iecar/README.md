# 2021-F drug_car PlatformIO 工程

本仓库包含电赛小车三端代码：STM32F407 主控、K210 识别端、OpenMV 巡线端。

## 目录

| 路径 | 说明 |
|---|---|
| `stm32f407_platformio/` | STM32F407 PlatformIO 工程，含 `car1` / `car2` / `native` 环境 |
| `k210_code/` | K210 目标识别脚本、模型和标签，运行时导入生成的 `protocol_contract.py` |
| `openmv_code/` | OpenMV 巡线脚本，运行时导入生成的 `protocol_contract.py` |
| `tools/` | 协议生成、协议回放、黄金帧验证、静态检查脚本 |
| `docs/` | 协议规格、架构边界、维护地图、迁移、legacy 与 full-closure 状态说明 |

## 构建与验证

```bash
./tools/check_static.sh quick     # Python 协议/状态回放 + 生成物同步 + 共享实现边界 + include 解析
./tools/check_static.sh host      # host C 协议核心测试
./tools/check_static.sh strict    # quick + host-C + pio run -e car1 + pio run -e car2 + pio test -e native
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

## 当前架构落地重点

1. 项目路径已收敛到 ASCII：`k210_code/`、`openmv_code/`、`openmv_code/line_follow.py`。
2. USART2 中断只提交接收数据；CarLink 解析、事件投递和 ACK 发送由主循环 service 处理。
3. K210/OpenMV/CarLink 接收路径具备固定帧流式重同步能力，并保留底层 fixed-buffer parser 校验。
4. Task1/Task2/Task3 终态通过 task-id-aware runtime helper 记录安全停车或正常完成。
5. car2 USART1 no-ACK CarLink 入口是有意保留的调试兼容入口，由角色配置开关控制。
6. OpenMV line-lost 低速找线是确认策略，已显式配置化并记录连续丢线计数。
7. 诊断 ring buffer 增加可消费出口，支持最近记录复制、格式化和 OLED 摘要显示。

## Full-closure 状态

当前方案闭环状态记录在 `docs/full_closure_status.md`。该文档只声明静态/脚本层面已闭合的事项；PlatformIO strict 与实机结果必须由对应环境验证。


Validation note: `tools/check_include_resolution.py` is part of quick/static validation and guards the car1/car2 shared-source include graph, including COMMON/PLATFORM and ROLE_SHARED strategy include paths.


Host-C protocol regression is available as `./tools/check_static.sh host` or `./tools/run_host_c_tests.sh`; strict mode invokes it after PlatformIO preflight on hosts with `pio`.
