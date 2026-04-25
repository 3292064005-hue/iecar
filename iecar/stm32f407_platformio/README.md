# STM32F407 PlatformIO 工作区

这是从原工程整理出的 PlatformIO 工作区，保留双车分环境与 native host 测试环境：

- `car1`：一号车固件
- `car2`：二号车固件
- `native`：host-level 协议/策略测试

## 使用方法

1. 用 VS Code + PlatformIO 打开 `stm32f407_platformio` 文件夹。
2. 构建车端固件：

```bash
pio run -e car1
pio run -e car2
```

3. 运行 native 测试：

```bash
pio test -e native
```

4. 烧录示例：

```bash
pio run -e car1 -t upload
```

## 验证入口

仓库根目录提供统一入口：

```bash
./tools/check_static.sh quick
./tools/check_static.sh strict
```

`quick` 不依赖 PlatformIO，覆盖协议生成物同步、黄金帧、wrapper 边界、状态回放和 host C 测试。`strict` 在 quick 后继续运行 `pio run -e car1`、`pio run -e car2`、`pio test -e native`。

## 迁移说明

- 当前 PlatformIO 工程使用 `ststm32` 平台与 `spl` 框架。
- 板卡基线使用 `black_f407zg`，匹配 STM32F407ZGT6 168MHz / 1MB Flash。
- 工程保留原双车业务代码与 `COMMON` 共享层，车级 wrapper 仍通过 `#include "../../../COMMON/..."` 复用共享实现。
- `COMMON/*.c` 不应再被单独加入编译列表，防止重复符号。
- 协议常量由 `tools/protocol_contract.py` 生成，车端默认配置包含 `src/COMMON/CHE/protocol_contract_generated.h`。

## 已知边界

- 当前交付环境没有 PlatformIO/arm-none-eabi toolchain，因此本包内只完成 quick 验证；真实固件构建需在安装 PlatformIO 的开发机执行 strict。
- 如果本地首次打开后发现 SPL 包与本地头文件有冲突，优先保持 `stm32f4xx_conf.h` 为项目版本，并不要把旧 `FWLIB/src` 再加入构建。
