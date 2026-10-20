# 编译与测试

← 返回 [`AGENTS.md`](../../AGENTS.md)

---

## 构建定位

本仓是 OpenHarmony 的一个部件，不能独立构建，源码树中的位置为 `<OHOS_ROOT>/base/telephony/sms_mms`。

| 项 | 值 |
|---|---|
| `part_name` | `sms_mms` |
| `subsystem_name` | `telephony` |
| 服务库 | `//base/telephony/sms_mms:tel_sms_mms` |
| 客户端库 | `//base/telephony/sms_mms/frameworks/native:tel_sms_mms_api` |
| 彩信编解码源码集 | `//base/telephony/sms_mms/frameworks/native/mms:mms_native_source` |
| ArkTS 1.1 模块 | `//base/telephony/sms_mms/frameworks/js/napi/:sms` |
| ArkTS 1.2 模块 | `//base/telephony/sms_mms/frameworks/ets/taihe:telephony_sms_taihe` |
| 仓颉 FFI | `//base/telephony/sms_mms/frameworks/cj:cj_sms_ffi` |
| 短码规则 | `//base/telephony/sms_mms/services/etc:sms_short_code_rules` |
| 测试组 | `test:unittest`、`test/fuzztest:fuzztest` |

### 编译选项

服务库开启 `cfi`、`cfi_cross_dso`、`branch_protector_ret = "pac_ret"`，优化选项 `-Os`。

`-fno-exceptions` 只出现在测试 gni 的 `sms_mms_cflags` 中，服务 `BUILD.gn` 没有该选项。服务与框架源码中没有 `try`、`catch`、`dynamic_cast`，这是编码约定而非编译器强制；新增代码保持不用，否则测试目标编译失败。

### 静态检查工具

本仓没有部件内的 lint、格式化配置或静态分析脚本（无 `.clang-format`、`.clang-tidy`、`cpplint` 配置）。代码风格与静态检查由 OpenHarmony 门禁流水线执行，本地以编译告警与本文「无构建环境时的静态自检」替代，不要查找不存在的本地 lint 命令。

### 构建命令

```shell
# 整个部件
./build.sh --product-name <product> --build-target sms_mms --ccache

# 服务库
./build.sh --product-name <product> --build-target "//base/telephony/sms_mms:tel_sms_mms"

# 全部单测
./build.sh --product-name <product> --build-target "//base/telephony/sms_mms/test:unittest"

# 单个测试目标
./build.sh --product-name <product> \
  --build-target "//base/telephony/sms_mms/test/gtest:unittest_mms_gtest"
```

`<product>` 取决于开发板，常见 `rk3568`。

---

## 两份源文件清单

| 清单 | 用途 |
|---|---|
| 根目录 `BUILD.gn` 的 `sources` 及其条件块 | 服务库 `tel_sms_mms` |
| `test/sms_mms_test.gni` 的 `sms_mms_src` | 静态库 `tel_sms_mms_test_base`，全部 gtest 目标链接它 |

测试清单除服务源文件外，还直接包含彩信编解码库与客户端 SDK 的源文件，以及测试辅助文件；服务库通过依赖获得这两部分。

新增源文件清单：

1. 服务源文件：根目录 `BUILD.gn` 与 `test/sms_mms_test.gni` 都加；受 feature 控制的放进两边对应条件块。
2. 彩信编解码源文件：`frameworks/native/mms/BUILD.gn` 与 `test/sms_mms_test.gni` 都加。
3. 客户端源文件：`frameworks/native/BUILD.gn` 与 `test/sms_mms_test.gni` 都加。
4. 新增 include 目录、外部依赖：两边同步，外部依赖另加 `bundle.json` 的 `deps.components`。

漏加测试清单的表现：服务库编译正常，gtest 链接报未定义符号。

---

## 测试目标

### gtest 模板

`test/gtest/BUILD.gn` 用 `foreach` 为 `test_sources` 中的每个源文件生成一个 `ohos_unittest`：

| 源文件 | 目标名 |
|---|---|
| `zero_branch_sms_part_test.cpp` | `tel_sms_mms_gtest`（唯一例外） |
| 其他 `xxx.cpp` | `unittest_xxx`，例如 `mms_gtest.cpp` → `unittest_mms_gtest` |
| `sms_satellite_gtest.cpp` | `unittest_sms_satellite_gtest`，仅 `sms_mms_satellite` 为真时加入 |

- 全部目标链接 `tel_sms_mms_test_base` 与 `core_service:ffrt_mocked`，`test_module` 均为 `tel_sms_mms_gtest`。
- **新增 gtest 文件必须加入 `test_sources`**，否则不生成目标。
- 测试文件以 `#define private public` 与 `#define protected public` 访问私有成员。
- 替身文件在 `test/gtest/mock/`（不在 `test/mock/`）：`mock_data_share_helper.h`、`mock_data_share_result_set.h`、`mock_sim_manager.h`、`mock_ims_sms_interface.h`、`mock_ims_sms_callback_interface.h`、`mock_ims_core_service_interface.h`、`mock_remote_object.h`。控制 DataShare 查询返回行数的写法，参考 `zero_branch_sms_part_test.cpp` 中 `CombineMultiPageMessage` 相关用例。
- `test/gtest/BUILD.gn` 中各目标由同一模板生成，样板高度重复；修改单个目标时，以 `sources` 中的文件名作为定位锚点。
- `test/unittest/` 下是 `ohos_executable("tel_sms_mms_test")`，是交互式手工测试程序，不属于 `unittest` 组。

### 目标与模块对应

| 目标 | 覆盖 |
|---|---|
| `unittest_mms_gtest`、`unittest_zero_branch_mms_test`、`unittest_mms_cdma_gtest` | 彩信编解码 |
| `unittest_sms_services_mms_gtest`、`unittest_mms_receiver_mgr_gtest` | 彩信事务与管理器 |
| `unittest_sms_gsm_gtest`、`unittest_zero_branch_sms_test`、`tel_sms_mms_gtest`、`unittest_zero_branch_sms_part1_test` | GSM 编解码、发送、接收、服务 |
| `unittest_cdma_sms_gtest`、`unittest_zero_branch_cdma_sms_test` | CDMA |
| `unittest_cb_gtest`、`unittest_zero_branch_cb_test` | 小区广播 |
| `unittest_sms_mms_receive_gtest` | 接收 |
| `unittest_sms_gtest`、`unittest_sms_service_proxy_gtest` | 服务接口、代理 |
| `unittest_sms_utils_gtest`、`unittest_zero_branch_utils_test`、`unittest_sms_mms_common_test` | 工具与公共代码 |
| `unittest_sms_ims_service_interaction_gtest` | IMS 短消息 |

前端（napi、taihe、cj）没有单元测试，前端改动写明静态自检与设备验证方式。

模糊测试有 23 个目标，`test/fuzztest/common_fuzzer/` 只提供共用代码。新增 fuzzer 目录后必须在 `test/fuzztest/BUILD.gn` 登记。

### 运行

```shell
hdc shell /data/test/unittest_mms_gtest
hdc shell /data/test/unittest_mms_gtest --gtest_filter=MmsGtest.<用例名>
hdc shell /data/test/unittest_mms_gtest --gtest_list_tests
```

### 用例编写约定

- 用例前三行注释：`@tc.number`、`@tc.name`、`@tc.desc`。
- 解码类用例构造最小字节序列：`std::make_unique<char[]>(n)` 填充字节后 `MmsDecodeBuffer::WriteDataBuffer`，断言返回值、输出值与缓冲区位置。
- 边界类用例覆盖空、1、上限、上限加一；段数类覆盖 127、128、255。
- 必须包含实际断言，不要只验证不崩溃。

---

## 无构建环境时的静态自检

| # | 检查 | 方法 |
|---|---|---|
| 1 | 新增 `.cpp` 进入对应 `BUILD.gn` 与 `test/sms_mms_test.gni` | 两处搜索文件名 |
| 2 | 新增 gtest 文件进入 `test/gtest/BUILD.gn` 的 `test_sources` | 搜索文件名 |
| 3 | 新增宏在 `smsmms.gni`、`bundle.json`、`BUILD.gn`、测试 gni 中齐全 | 四处搜索 |
| 4 | 按输入长度读取、循环、分配前有比较 | 追溯长度来源 |
| 5 | 计数、序号、长度变量类型能容纳取值范围 | 核对声明类型 |
| 6 | 长度校验用上限，容量用上限加一 | 核对常量用法 |
| 7 | 指针参数判空，`char*` 构造字符串前判空 | 核对入口 |
| 8 | 解码函数的每个 `return false` 对应真实错误 | 对照规范 |
| 9 | 修改基础编码函数返回语义时，全部调用点已列出 | 搜索函数名 |
| 10 | 发送路径每个失败返回前回调结果 | 逐个核对 `return` |
| 11 | 三套前端同步 | 搜索接口名 |
| 12 | 新增接口按「权限收口」校验 | 核对 `SmsService` |
| 13 | 日志无号码、正文、彩信内容 | 核对新增 `TELEPHONY_LOG*` |

---

## DoD

完整 DoD 见 [`AGENTS.md`](../../AGENTS.md)「最小验证闭环」。最小要求：

- [ ] 本文静态自检全部通过。
- [ ] 有构建环境时，`sms_mms` 部件与相关 gtest 目标编译通过并运行全绿。
- [ ] 至少一条带断言的用例覆盖改动分支，包含边界值。
- [ ] [`high-risk.md`](high-risk.md)「变更风险矩阵」中的对应项已确认。
