# 边界与约束

← 返回 [`AGENTS.md`](../../AGENTS.md)

---

## 模块依赖方向

```text
interfaces/kits/js (d.ts)
      │
frameworks/js/napi   frameworks/ets/taihe   frameworks/cj          三套前端
      └───────────────┬──────────────────┘
                      ▼
frameworks/native/sms  客户端 SDK（tel_sms_mms_api）   frameworks/native/mms  彩信编解码库
                      │ IPC SA 4008                         ▲ 前端与服务都链接
                      ▼                                     │
services/sms  Stub → SmsService → SmsInterfaceManager → 各管理器
services/mms  彩信事务
utils/        文本编码、公共工具、打点
```

| 规则 | 说明 |
|---|---|
| 彩信编解码库同时运行在应用进程与服务进程 | 修改会同时影响 `decodeMms`、`encodeMms` 与服务侧处理，两侧都要验证 |
| IPC 定义在本仓 | `interfaces/innerkits/sms_service_ipc_interface_code.h`，代理与 Stub 都在本仓，改动在本仓内闭合 |
| Stub 只做卡槽校验、反序列化、转发 | 权限在 `SmsService` |
| 服务按卡槽持有 `SmsInterfaceManager` | 管理器集合在 `InitModule` 中创建一次 |
| `services/` 不依赖前端 | 前端通过客户端 SDK 访问服务 |

---

## 不可信输入处理规则

本仓的输入来源：网络侧 PDU（短消息、小区广播、WAP Push）、彩信服务器 HTTP 响应、应用传入的 PDU 与参数、仓颉与 ArkTS 前端的原始指针与数组。仓内历史上的安全修复集中在这些入口。

| 规则 | 检查方法 |
|---|---|
| 按输入中的长度或计数读取前，先与剩余缓冲区长度比较 | 每个 `GetOneByte`、`memcpy_s`、下标访问，追溯长度来源 |
| 来自输入的循环次数设置上限 | 对条目数、段数、字段数，确认存在与协议上限或剩余长度的比较 |
| 计数、序号、长度变量的类型能容纳取值范围 | 段数与段序号使用 `int` 或 `uint8_t`，禁止 `int8_t`；累加计数同样适用 |
| 缓冲区容量与长度上限区分 | 数组按 `MAX_USER_DATA_LEN + 1` 分配留出结束符；长度校验写 `size > MAX_USER_DATA_LEN` |
| 有符号与无符号不混比 | 下标减偏移后先判断非负，再与 `size()` 比较 |
| 指针参数判空 | FFI 的 `CArrXxx.head`、`char*`；工具函数的 `const uint8_t *data` |
| 彩信 PDU 与 HTTP 响应长度有上限 | `MmsNetworkClient` 中与 `MMS_PDU_MAX_SIZE`、`SEND_CONF_MAX_SIZE` 的比较 |
| 解码失败只影响当前消息 | 返回错误并丢弃，不崩溃，不影响后续消息 |

边界值用例的最小集合：空、1、上限、上限加一；段数类另加 127、128、255。

---

## 彩信编解码约束

分层：

```text
MmsMsg::DecodeMsg
  └─ MmsHeader::DecodeMmsHeader          字段码 → memberFuncMap_ 中的字段解码函数
       └─ DecodeFieldXxx                  各类字段
            └─ MmsEncodeString / MmsDecodeBuffer / MmsContentType   基础编码
  └─ MmsBody::DecodeMmsBody              多部分正文
```

失败语义：

- `DecodeMmsHeader` 循环中，**任一字段解码函数返回 `false`，整个头部解码立即返回失败**，整条彩信无法展示。未登记的字段码按未知字段跳过。
- 因此字段解码函数的 `false` 只表示真正的格式错误或越界。协议允许的特殊形式（零长度字符串、只有结束符的文本、可选参数缺省）必须返回 `true` 并给出空值。
- 基础编码函数的返回值被多处调用，有的调用点检查返回值，有的不检查。修改基础函数的返回语义前，搜索全部调用点，列出受影响字段。
- 判断一个形式是否合法，以 OMA MMS 编码规范与 WSP 规范为准，并用最小字节序列写成用例。

编码侧约束：`EncodeMmsHeader` 对必选字段缺失返回失败，这是正确行为，不要为兼容而放宽。

---

## 彩信事务约束

- 下载与发送由 `MmsReceive`、`MmsSender` 执行，各自有互斥锁串行化事务；数据网络由 `MmsNetworkManager` 申请，HTTP 由 `MmsNetworkClient` 执行。
- 事务结果通过 DataShare `mms_pdu` 表与调用方交换 PDU。
- **URL 与地址的安全校验归属服务器侧。** `MmsReceive::ExecuteDownloadMms` 的注释说明了客户端不做 URL 校验的理由，仓内曾加入客户端校验后回退。在事务路径上新增或删除这类校验属于需要人工确认的改动。
- HTTP 响应的累计大小限制保持有效，修改限制值属于高危改动。

---

## 发送链路约束

- `SmsSendManager` 通过 `SmsNetworkPolicyManager` 获取网络制式，选择 GSM 或 CDMA 发送器；网络制式未知时直接失败并回调。
- `SmsSender` 是 `TelEventHandler`，发送请求记录在 `sendCacheMap_`，以引用号对齐 RIL 响应；重试上限 `MAX_SEND_RETRIES`。
- 发送域：IMS 已注册且 IMS 短消息开关打开时走 IMS，否则走 CS；CS 发送要求语音服务在服务状态。
- **每个发送请求最终必须回调一次发送结果。** 每条失败返回路径都调用 `SendResultCallBack`，否则应用侧一直等待。
- 长短消息拆分见「长短消息约束」。

## 接收链路约束

- 接收处理器是 `TelEventHandler`，接收期间持有运行锁，超时自动释放。
- DataShare 未就绪时：先回复 SMSC 确认，消息入队，就绪后 `HandleMessageQueue` 补处理。**已确认的消息不能丢。**
- 分段先写入 `sms_subsection` 表，全部到齐后合并；段序号越界时删除该组分段记录。
- 合并成功后发布 `COMMON_EVENT_SMS_RECEIVE_COMPLETED`，WAP Push 走独立事件。

## 长短消息约束

| 侧 | 位置 | 约束 |
|---|---|---|
| 发送拆分 | `GsmSmsSender::TextBasedSmsSplitDelivery`、`GsmSmsMessage` 拆分函数 | 每段编码后长度 ≤ `MAX_USER_DATA_LEN`；段数写入 8 位字段；所有段共享同一个 8 位引用号 |
| 接收合并 | `SmsReceiveHandler::CombineMessagePart`、`CombineMultiPageMessage` | 段数最大 255；计数变量与段数同宽或更宽；段序号从 1 开始，减偏移后校验范围 |
| 合并完成判定 | 同上 | 查到的段数、非空段数都与段数相等才合并 |

CDMA 接收处理器复用基类的 `CombineMessagePart`，合并侧的修复对两种制式同时生效；CDMA 拆分在 `cdma/cdma_sms_message.cpp`，修改 GSM 拆分时检查 CDMA 拆分是否存在同一问题。

### 已知技术债

以下位置在 master 上与本文约束不一致，改动相邻代码时一并评估，不要作为先例：

| 位置 | 与约束的差异 |
|---|---|
| `SmsReceiveHandler::CombineMultiPageMessage` 的循环计数变量 `count` | 类型为 `int8_t`，与「不可信输入处理规则」中的整数宽度要求不符 |
| `SmsSender::DataBasedSmsDeliverySplitPage` 的长度校验 | 写作 `> MAX_USER_DATA_LEN + 1`，与「容量与上限区分」不符 |
| `GsmSmsSender::TextBasedSmsSplitDelivery` 的长度校验失败分支 | 直接返回，没有回调发送结果，与「发送链路约束」不符 |
| `cdma/cdma_sms_sub_parameter.cpp` 解码侧的 `udhBytes`、`udBytes` 校验 | 上限写作 `MAX_USER_DATA_LEN + 1`，需要对照 3GPP2 规范确认后再改，避免误拒合法消息 |

---

## 前端约束

| 前端 | 位置 | 约束 |
|---|---|---|
| ArkTS 1.1 | `frameworks/js/napi/` | 参数校验失败抛参数错误；异步工作在完成回调中释放上下文 |
| ArkTS 1.2 | `frameworks/ets/taihe/` | 接口形状以 `.taihe` IDL 为准，行为在手写实现中；taihe 工具链生成的胶水代码位于构建输出目录，不入库，不要手改生成物 |
| 仓颉 | `frameworks/cj/src/sms_ffi.cpp` | 导出函数的数组头指针与字符串指针判空；数组长度设上限；`malloc` 分配的返回内存由仓颉侧释放，失败路径释放已分配部分 |

- `napi_send_recv_mms.cpp` 与 `ani_send_recv_mms.cpp` 用静态计数与互斥锁限制并发彩信事务，修改一侧时同步另一侧。
- 三套前端对同一接口的参数校验保持一致，任一前端新增校验时检查另外两套。

---

## 权限收口

权限与系统应用校验全部在 `SmsService` 的接口实现中：

| 接口类别 | 要求 |
|---|---|
| 发送短消息 | `SEND_MESSAGES` |
| SIM 卡短消息增删改 | 系统应用 + `RECEIVE_MESSAGES` + `SEND_MESSAGES` |
| SIM 卡短消息查询、小区广播配置 | 系统应用 + `RECEIVE_MESSAGES` |
| 短消息中心、默认卡槽、IMS 短消息开关设置 | 系统应用 + `SET_TELEPHONY_STATE` |
| 状态查询类 | 系统应用 + `GET_TELEPHONY_STATE` |
| 发送彩信 | 系统应用 + `SEND_MESSAGES` |
| 下载彩信 | 系统应用 + `RECEIVE_MMS` |

新增接口参照同类接口复制校验顺序：先系统应用，再权限。

---

## 新增接口清单

1. `@ohos.telephony.sms.d.ts` 声明，补齐 `@permission`、`@throws`、`@syscap`、`@since`。
2. napi、taihe IDL 与实现、cj FFI 三套前端。
3. 客户端 `SmsServiceManagerClient`、代理 `SmsServiceProxy`。
4. `SmsServiceInterfaceCode` 末尾追加接口码。
5. `SmsInterfaceStub` 登记处理函数，读取顺序与代理写入一致，需要卡槽的先校验。
6. `SmsService` 实现，按「权限收口」校验。
7. 管理器实现。
8. gtest 用例覆盖权限拒绝与正常路径；对外 IPC 接口补充 fuzzer 并在 `test/fuzztest/BUILD.gn` 登记。
