# PlatformIO 迁移说明

## 当前入口

- 工程目录：`stm32f407_platformio/`
- car1：`pio run -e car1`
- car2：`pio run -e car2`
- host 测试：`pio test -e native`

## 验证模式

```bash
./tools/check_static.sh quick
./tools/check_static.sh strict
```

`quick` 执行 Python 协议黄金帧、状态回放、共享实现边界检查和 host C 测试；`strict` 在 quick 通过后执行 PlatformIO 构建和 native 测试。

## 回滚方式

1. 若 strict 构建暴露 SPL/工具链兼容问题，先保留业务代码，修正 `platformio.ini` 与 include path。
2. 若 CarLink 新协议与旧固件不兼容，可临时在 `car_project_config_shared.h` 中打开兼容宏：`CAR_PROTOCOL_COMPAT_V1`、`CAR_LINK_COMPAT_TASK_FIELD`。
3. 若现场需要保守巡线起步，可调整 `CAR_OPENMV_REQUIRE_FIRST_FRAME`，但默认应保持首帧闸门开启。


## Strict environment manifest

`tools/platformio_strict_manifest.json` is the release-gate manifest. `strict` mode performs a preflight check before building:

- PlatformIO CLI must be present and within the supported 6.x range.
- `platformio.ini` must expose `car1`, `car2`, and `native` environments.
- Firmware environments must use the pinned STM32 platform constraint `platformio/ststm32@^19.0.0`.
- Missing PlatformIO is a hard failure in `strict`; it is never reported as a successful firmware validation.

## Shared role source layout

`car1` and `car2` compile common role-level sources directly from `COMMON/ROLE_SHARED` and shared CHE sources directly from `COMMON/CHE`; role-local wrappers/adapters for those modules are not allowed.


Validation note: `tools/check_include_resolution.py` is part of quick/static validation and guards the car1/car2 shared-source include graph, including COMMON/PLATFORM and ROLE_SHARED strategy include paths.


Host-C protocol regression is available as `./tools/check_static.sh host` or `./tools/run_host_c_tests.sh`; strict mode invokes it after PlatformIO preflight on hosts with `pio`.
