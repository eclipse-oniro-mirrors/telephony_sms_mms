# telephony_sms_mms 知识库入口

本文件是本仓知识库的唯一入口，其余文档位于 `docs/agents/`，全部从这里路由进入。

使用顺序：先读「这个仓是什么」确定方向，再用「知识路由表」选中文档，修改前核对「红线」，完成前执行「最小验证闭环」。

> **说明：**
>
> 本知识库描述 master 主干的状态。文档中的不变量是主干应当成立的约束，旧分支上某条不变量尚未落地时，缺口本身往往就是待修复的缺陷。读到一条不变量后，先在当前代码中搜索它的落点；找不到落点时，结论是「这里缺了一处」，而不是「文档写错了」；找到落点但写法与不变量相反时，结论相同。描述现状的表格（测试目标配置、文件清单、已知技术债）同样以 master 为准，其他分支上以实际代码为准。

---

## 这个仓是什么

OpenHarmony 电话子系统的**短消息与彩信**部件，`part_name` 为 `sms_mms`。

- 运行形态：系统服务 SA 4008，进程 `telephony`，产物 `libtel_sms_mms.z.so`。
- 职责：短消息收发、SIM 卡短消息、小区广播、WAP Push；彩信发送、下载与编解码。
- 上游依赖：core_service（RIL、卡状态）、telephony_data（DataShare 数据库）、netmanager 与 netstack（彩信数据网络与 HTTP）、IMS 服务。
- 下游消费者：应用（`@ohos.telephony.sms`，含 ArkTS 1.1、1.2 与仓颉三套前端）、短消息应用（数据库与公共事件）。

心智模型：**短消息是每卡槽一套发送、接收、杂项管理器；彩信是一次 HTTP 事务加一个编解码库。** 本仓处理的 PDU、彩信内容、FFI 参数都来自不可信来源，缺陷多出在解码边界、整数宽度、长度校验与指针判空。

主链路的完整图示见 [`docs/agents/code-map.md`](docs/agents/code-map.md)「主链路」，骨架如下：

```text
发送：前端 → 客户端 → Stub（卡槽校验）→ SmsService（权限）→ SmsInterfaceManager → SendManager → Gsm/CdmaSmsSender → IMS 或 CS
接收：RIL → Gsm/CdmaSmsReceiveHandler → DataShare 分段表 → 合并 → 公共事件
彩信：下载经 MmsNetworkClient（HTTP）；解码在调用方进程内经 MmsMsg → MmsHeader → MmsBody
```

---

## 知识路由表

### 任务触发路由

| 任务 | 先读 | 再改 |
|---|---|---|
| 彩信内容解析失败、解码异常 | [`boundaries.md`](docs/agents/boundaries.md)「彩信编解码约束」 | `frameworks/native/mms/src/` |
| 彩信下载、发送失败 | [`boundaries.md`](docs/agents/boundaries.md)「彩信事务约束」 | `services/mms/` |
| 长短消息拆分、合并异常 | [`boundaries.md`](docs/agents/boundaries.md)「长短消息约束」 | `gsm/gsm_sms_sender.cpp`、`sms_receive_handler.cpp` |
| PDU 编解码、越界、崩溃 | [`boundaries.md`](docs/agents/boundaries.md)「不可信输入处理规则」 | `services/sms/gsm/`、`services/sms/cdma/` |
| 短消息收不到、重复收到 | [`boundaries.md`](docs/agents/boundaries.md)「接收链路约束」 | `sms_receive_handler.cpp`、`sms_persist_helper.cpp` |
| 发送结果回调不回、重发异常 | [`boundaries.md`](docs/agents/boundaries.md)「发送链路约束」 | `sms_sender.cpp`、`sms_send_manager.cpp` |
| 仓颉、ArkTS 前端崩溃或参数问题 | [`boundaries.md`](docs/agents/boundaries.md)「前端约束」 | `frameworks/cj/`、`frameworks/js/napi/`、`frameworks/ets/taihe/` |
| 小区广播、紧急广播 | [`high-risk.md`](docs/agents/high-risk.md)「高危入口」 | `gsm/gsm_sms_cb_handler.cpp`、`sms_misc_manager.cpp` |
| 新增 JS 接口或 IPC 接口 | [`boundaries.md`](docs/agents/boundaries.md)「新增接口清单」 | 声明、三套前端、客户端、Stub、服务 |
| 权限与系统应用限制 | [`boundaries.md`](docs/agents/boundaries.md)「权限收口」 | `services/sms/sms_service.cpp` |
| 新增源文件、宏、测试 | [`build-and-test.md`](docs/agents/build-and-test.md)「两份源文件清单」 | `BUILD.gn`、`test/sms_mms_test.gni`、`test/gtest/BUILD.gn` |
| 按现象定位根因 | [`failure-modes.md`](docs/agents/failure-modes.md)「快速分诊表」 | 无 |
| 评估改动风险 | [`high-risk.md`](docs/agents/high-risk.md)「变更风险矩阵」 | 无 |
| 排查线上问题 | [`observability.md`](docs/agents/observability.md) | 无 |

### 路径触发路由

| 文件路径 | 必读知识 |
|---|---|
| `frameworks/native/mms/**` | `boundaries.md`「彩信编解码约束」「不可信输入处理规则」 |
| `services/mms/**` | `boundaries.md`「彩信事务约束」、`high-risk.md`「高危入口」 |
| `services/sms/gsm/**`、`services/sms/cdma/**` | `boundaries.md`「不可信输入处理规则」「长短消息约束」 |
| `services/sms/sms_receive_handler.cpp` | `boundaries.md`「接收链路约束」「长短消息约束」 |
| `services/sms/sms_sender.cpp`、`sms_send_manager.cpp` | `boundaries.md`「发送链路约束」 |
| `services/sms/sms_service.cpp` | `boundaries.md`「权限收口」 |
| `services/sms/sms_interface_stub.cpp`、`interfaces/innerkits/*ipc_interface_code.h` | `boundaries.md`「新增接口清单」 |
| `frameworks/cj/**`、`frameworks/js/napi/**`、`frameworks/ets/taihe/**` | `boundaries.md`「前端约束」 |
| `utils/text_coder.cpp` | `boundaries.md`「不可信输入处理规则」 |
| `BUILD.gn`、`smsmms.gni`、`test/**` | `build-and-test.md` |

### 词汇触发路由

| 词 | 含义 | 落点 |
|---|---|---|
| PDU | 协议数据单元，短消息与彩信的原始字节 | 编解码全仓 |
| TP-UD / user data | 短消息用户数据，单段上限 `MAX_USER_DATA_LEN` | `sms_pdu_code_type.h` |
| 分段 / concat / msgCount / seqId | 长短消息的段数与段序号，段数由 PDU 头给出，取值可到 255 | 发送拆分、接收合并 |
| SMSC | 短消息中心 | `SmsMiscManager` |
| CB / ETWS / CMAS | 小区广播、地震海啸预警、商用移动预警 | `gsm_cb_*` |
| WAP Push | 通过短消息承载的推送，彩信通知经此到达 | `sms_wap_push_handler.cpp` |
| M-Notification-Ind / M-Retrieve-Conf | 彩信通知与彩信内容 PDU | `MmsMsg` |
| Encoded-string-value | 彩信头部带字符集的字符串字段 | `MmsEncodeString` |
| Value-length / Uintvar | WSP 基础长度编码 | `MmsDecodeBuffer` |
| IMS 短消息域 | IMS 注册后经 IMS 发送，否则经 CS | `SmsSender`、`SmsNetworkPolicyManager` |
| DataShare 就绪 | 数据库可写之前收到的短消息先确认后入队 | `SmsReceiveHandler` |
| 短码 | 付费或特服短号码，发送前按规则分类 | `sms_short_code_matcher.cpp` |
| taihe | ArkTS 1.2 的接口 IDL 与实现 | `frameworks/ets/taihe/` |

### 动手前必须先自陈

写出第一行代码改动之前，先在回复中用三行写清楚：

```text
任务类别：<「任务触发路由」表中的哪一行>
已读文档：<docs/agents/ 下实际读过的文件与小节>
命中约束：<「红线」编号 + 分层文档中找到的不变量；确认没有则写"无">
```

写不出「命中约束」时，说明尚未掌握本次改动的影响范围，不要开始修改。

---

## 红线

违反以下任一条的改动不可合入。

1. **禁止未校验长度就按输入中的长度或计数读取、循环、分配。** PDU、彩信、HTTP 响应、FFI 数组都是不可信输入。
2. **禁止用窄于取值范围的整数类型保存来自输入的计数、序号、长度。** 段数最大 255，`int8_t` 在 128 起溢出。
3. **禁止把缓冲区容量常量与数据长度上限混用。** 缓冲区按「上限 + 1」分配以容纳结束符，长度校验用上限本身。
4. **禁止对外部传入的指针不判空。** FFI 导出函数、NAPI 入口、公共工具函数的指针参数一律判空。
5. **禁止彩信字段解码函数对合法的空值或可选形式返回失败。** 头部解码中任一字段返回失败会中止整条彩信的解码。
6. **禁止在 `SmsService` 之外做权限判断，或新增接口不做权限与系统应用校验。**
7. **禁止打印号码、短消息正文、彩信内容。** 日志中的这类字段使用 `%{private}` 或不输出。
8. **禁止在接收路径上丢弃已向网络确认的短消息。** DataShare 未就绪时入队，就绪后补处理。
9. **禁止只改一套前端。** napi、taihe、cj 三套前端的参数校验与行为保持一致。
10. **禁止新增 `.cpp` 只加入一份源文件清单。** 服务 `BUILD.gn` 与 `test/sms_mms_test.gni` 各有一份。
11. **禁止修改 `SmsServiceInterfaceCode` 已有枚举值。** 只能追加。

### 必须停下来找人确认的改动

| 触发条件 | 原因 |
|---|---|
| 改动紧急小区广播的接收、过滤或展示路径 | 公共安全告警 |
| 在彩信下载或发送路径上新增或删除 URL、地址校验 | 安全边界的归属已有结论，见 [`boundaries.md`](docs/agents/boundaries.md)「彩信事务约束」 |
| 修改 `@ohos.telephony.sms.d.ts` 已发布接口的签名或语义 | 应用兼容性 |
| 收紧或放宽权限、系统应用校验 | 安全事件 |
| 修改短码规则文件或短码分类逻辑 | 付费短消息拦截 |
| 修改 DataShare 表结构、URI 或字段含义 | 与短消息应用、telephony_data 共同约定 |
| 修改 feature 宏默认值 | 影响所有未显式配置的产品 |
| 新增或替换外部部件依赖、第三方库（`external_deps`、`bundle.json` 的 `deps`） | 许可证与供应链合规审查，改变部件依赖关系 |

判据：这个改动的后果能不能在本仓内验证？不能，就属于本节。

---

## 最小验证闭环

详细命令见 [`docs/agents/build-and-test.md`](docs/agents/build-and-test.md)。

### 静态自检

必做，无需构建环境：

- 每个读取、循环、分配的长度或计数，追溯到其校验点。
- 每个保存计数、序号、长度的变量，类型能容纳取值范围，比较时无符号与有符号不混用。
- 每个指针参数判空，覆盖 FFI、NAPI、工具函数入口。
- 解码函数的每个 `return false` 对应真实的格式错误或越界，而不是合法的特殊形式。
- 新增 `.cpp` 同时进入 `BUILD.gn` 与 `test/sms_mms_test.gni`；新增 gtest 文件进入 `test/gtest/BUILD.gn` 的 `test_sources`。
- `build-and-test.md`「无构建环境时的静态自检」逐项通过。

### 构建与测试

```shell
./build.sh --product-name <product> --build-target sms_mms
./build.sh --product-name <product> --build-target "//base/telephony/sms_mms/test:unittest"
hdc shell /data/test/unittest_mms_gtest --gtest_filter=<用例名>*
```

### DoD 判定

- [ ] 目标行为在最小复现场景下被验证，场景写入提交说明；解码类问题附最小 PDU 字节序列。
- [ ] 至少一条带实际断言的用例覆盖改动分支，包括边界值（上限、上限加一、空、最大段数）。
- [ ] 回归面已按 [`high-risk.md`](docs/agents/high-risk.md)「变更风险矩阵」确认。
- [ ] 日志中无号码、正文、彩信内容。

### 最终回复必须包含

1. 改了什么：文件清单，每处一句话说明为什么改在这里。
2. 「动手前必须先自陈」中的三行。
3. DoD 逐项结果：已验证或未验证，未验证写明原因。
4. 验证边界：没有 OpenHarmony 构建环境时，明说「仅完成静态自检，未编译、未运行测试」。
5. 遗留风险：需人工复核的项、发现但未修的技术债、同一模式在其他编解码文件中的残留。

---

## 分层文档索引

| 文档 | 内容 | 什么时候读 |
|---|---|---|
| [`docs/agents/code-map.md`](docs/agents/code-map.md) | 目录职责、主链路、关键类、DataShare 表、任务到路径 | 第一次进入某个模块 |
| [`docs/agents/boundaries.md`](docs/agents/boundaries.md) | 依赖方向、不可信输入规则、编解码约束、收发链路约束、前端约束、权限、接口清单 | 动手改代码前必读 |
| [`docs/agents/high-risk.md`](docs/agents/high-risk.md) | 高危入口、高危配置、高危事件序列、变更风险矩阵 | 改动涉及彩信、广播、权限、宏时 |
| [`docs/agents/failure-modes.md`](docs/agents/failure-modes.md) | 快速分诊表、失效模式、根因排查方法、修复反例、回归要求 | 排查缺陷时 |
| [`docs/agents/observability.md`](docs/agents/observability.md) | 日志标识、dump、关键日志、HiSysEvent、公共事件 | 定位现场问题 |
| [`docs/agents/build-and-test.md`](docs/agents/build-and-test.md) | 构建目标、两份源文件清单、gtest 模板、静态自检 | 验证阶段 |
