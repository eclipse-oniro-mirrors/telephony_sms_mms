# 代码地图

← 返回 [`AGENTS.md`](../../AGENTS.md)

---

## 部件定位

| 项 | 值 |
|---|---|
| 部件 | `sms_mms`，子系统 `telephony` |
| 系统服务 | SA 4008，进程 `telephony`，产物 `libtel_sms_mms.z.so`，依赖 SA 4010（core_service），依赖等待 60 秒 |
| 启动配置 | `sa_profile/4008.json` 常驻启动；`4008_dynamic.json` 按 modem 动态上下电参数按需启停 |
| 职责 | 短消息（SMS）收发、SIM 卡短消息管理、小区广播（CB）、WAP Push，彩信（MMS）的发送、下载与编解码 |
| 上游依赖 | core_service（RIL 与卡状态）、telephony_data（DataShare 数据库）、netmanager 与 netstack（彩信数据网络与 HTTP）、ims 服务（IMS 短消息） |
| 下游消费者 | 应用（`@ohos.telephony.sms`）、短消息应用（DataShare 表与公共事件） |

心智模型：短消息部分是**每卡槽一套发送、接收、杂项管理器**，由服务按卡槽号分发；彩信部分是**一次 HTTP 事务加一个编解码库**，编解码库同时运行在应用进程和服务进程。

## 嵌套指引

本仓没有目录级的 `AGENTS.md`。全部任务指引经由根目录 `AGENTS.md` 路由到 `docs/agents/` 下的分层文档。

## 目录职责

```text
telephony_sms_mms/
├── BUILD.gn / smsmms.gni       服务库 tel_sms_mms 与 feature 开关
├── bundle.json                 部件声明
├── sa_profile/                 SA 4008 启动配置
├── interfaces/
│   ├── kits/js/                @ohos.telephony.sms.d.ts
│   └── innerkits/              IPC 接口与接口码、客户端、ShortMessage、MmsMsg、错误码
│       ├── ims/                IMS 短消息客户端接口
│       └── satellite/          卫星短消息接口
├── frameworks/
│   ├── native/sms/             客户端 SDK：管理客户端、服务代理、回调桩
│   ├── native/mms/             彩信编解码库（source set mms_native_source）
│   ├── js/napi/                ArkTS 1.1：napi_sms、napi_mms、napi_send_recv_mms
│   ├── ets/taihe/              ArkTS 1.2：taihe IDL 与实现、ani_send_recv_mms
│   └── cj/                     仓颉 FFI
├── services/
│   ├── sms/                    短消息服务主体
│   │   ├── gsm/                GSM 编解码、发送器、接收处理器、小区广播处理
│   │   ├── cdma/               CDMA 编解码、发送器、接收处理器
│   │   ├── ims_service_interaction/     IMS 短消息客户端
│   │   └── satellite_service_interaction/  卫星短消息客户端
│   ├── mms/                    彩信发送、下载、数据网络、HTTP、APN、PDU 存取
│   ├── telephony_ext_wrapper/  扩展库动态加载
│   └── etc/                    短码规则 sms_short_code_rules.json
├── utils/                      文本编码、公共工具、HiSysEvent、策略工具
└── test/
    ├── gtest/                  白盒单元测试，链接 ffrt 替身；替身文件在 gtest/mock/
    ├── unittest/               交互式手工测试程序，不属于 unittest 组
    └── fuzztest/               模糊测试
```

## 主链路

### 发送短消息

```text
应用 sms.sendShortMessage
  └─ frameworks/js/napi/src/napi_sms.cpp（异步 work）
       └─ frameworks/native/sms/src/sms_service_manager_client.cpp → sms_service_proxy.cpp
            └─ IPC SA 4008 → services/sms/sms_interface_stub.cpp
                 OnRemoteRequest → memberFuncMap_ → 卡槽合法性校验
                 └─ services/sms/sms_service.cpp  权限校验（SEND_MESSAGES 等）
                      └─ SmsInterfaceManager（按卡槽）→ SmsSendManager::TextBasedSmsDelivery
                           SmsNetworkPolicyManager::GetNetWorkType 决定 GSM 或 CDMA
                           └─ GsmSmsSender / CdmaSmsSender（TelEventHandler）
                                分段编码 → SendSmsToRil：
                                IMS 域且 IMS 短消息开关打开 → IMS 通道；否则走 CS 通道
                                发送缓存 sendCacheMap_ 记录请求，响应到达后回调发送结果
```

### 接收短消息

```text
RIL 上报 → GsmSmsReceiveHandler / CdmaSmsReceiveHandler::ProcessEvent（TelEventHandler）
  └─ SmsReceiveHandler::HandleSmsEvent
       DataShare 未就绪：回复 SMSC 确认 → 消息入队 → 就绪后 HandleMessageQueue 补处理
       DataShare 就绪：HandleSmsByType → 分段写入 sms_subsection 表
       └─ CombineMessagePart：单段直接组装；多段 CombineMultiPageMessage 等待全部分段到齐
            └─ 发布 COMMON_EVENT_SMS_RECEIVE_COMPLETED（WAP Push 走 COMMON_EVENT_SMS_WAPPUSH_RECEIVE_COMPLETED）
  接收全程持有运行锁 RunningLock，超时后自动释放
```

### 下载与解码彩信

```text
应用 sms.downloadMms
  └─ IPC → SmsService::DownloadMms（系统应用 + RECEIVE_MMS）
       └─ MmsReceiveManager::DownloadMms → services/mms/mms_receive.cpp ExecuteDownloadMms
            └─ MmsNetworkManager 申请彩信数据网络 → MmsNetworkClient 发起 HTTP
                 PDU 通过 DataShare mms_pdu 表回传给调用方
应用 sms.decodeMms
  └─ frameworks/js/napi/src/napi_mms.cpp → frameworks/native/mms 编解码库
       MmsMsg::DecodeMsg → MmsHeader::DecodeMmsHeader（按字段码分发）→ MmsBody::DecodeMmsBody
```

彩信编解码库在应用进程内运行，解码失败只影响调用它的应用，不涉及服务进程。服务库通过 `frameworks/native/mms:mms_native_source` 也链接了同一套代码。

## 关键类

| 类 | 位置 | 说明 |
|---|---|---|
| `SmsService` | `services/sms/sms_service.cpp` | SystemAbility 与 IPC 实现，**鉴权收口** |
| `SmsInterfaceStub` | `services/sms/sms_interface_stub.cpp` | IPC 分发，按卡槽持有 `SmsInterfaceManager`，模块只初始化一次 |
| `SmsInterfaceManager` | `services/sms/sms_interface_manager.cpp` | 单卡槽的发送、接收、杂项、彩信管理器集合 |
| `SmsSendManager` | `services/sms/sms_send_manager.cpp` | 选择 GSM 或 CDMA 发送器，处理重发 |
| `SmsSender` | `services/sms/sms_sender.cpp` | 发送器基类，发送缓存、重试上限 3 次、IMS 与 CS 域切换 |
| `SmsReceiveHandler` | `services/sms/sms_receive_handler.cpp` | 接收处理基类，DataShare 就绪判断、分段合并、运行锁 |
| `SmsNetworkPolicyManager` | `services/sms/sms_network_policy_manager.cpp` | 网络制式、IMS 注册域、语音服务状态 |
| `SmsMiscManager` | `services/sms/sms_misc_manager.cpp` | 小区广播范围、SIM 卡短消息、短消息中心地址、默认卡槽 |
| `SmsPersistHelper` | `services/sms/sms_persist_helper.cpp` | 短消息数据库与联系人拦截名单访问 |
| `MmsReceive` / `MmsSender` | `services/mms/` | 彩信下载与发送事务 |
| `MmsMsg` / `MmsHeader` / `MmsBody` | `frameworks/native/mms/src/` | 彩信 PDU 编解码 |
| `MmsEncodeString` / `MmsDecodeBuffer` | 同上 | 编码字符串与解码缓冲区，字段解码的基础原语 |

## DataShare 表

| URI | 用途 |
|---|---|
| `datashare:///com.ohos.smsmmsability/sms_mms/sms_subsection` | 接收中的短消息分段 |
| `datashare:///com.ohos.smsmmsability/sms_mms/sms_mms_info` | 已完成的短消息 |
| `datashare:///com.ohos.smsmmsability/sms_mms/session` | 会话 |
| `datashare:///com.ohos.smsmmsability/sms_mms/mms_pdu` | 彩信 PDU |
| `datashare:///com.ohos.pdpprofileability/net/pdp_profile?simId=` | 彩信 APN |
| `datashare:///com.ohos.contactsdataability/...` | 联系人与拦截名单 |

## 任务到路径

| 任务 | 首先打开 |
|---|---|
| 新增或修改 JS 接口 | `interfaces/kits/js/@ohos.telephony.sms.d.ts`，`frameworks/js/napi/`，`frameworks/ets/taihe/` |
| 新增 IPC 接口 | 见 [`boundaries.md`](boundaries.md)「新增 IPC 接口的清单」 |
| 发送失败、发送结果回调异常 | `sms_send_manager.cpp`、`sms_sender.cpp`、`gsm/gsm_sms_sender.cpp`、`cdma/cdma_sms_sender.cpp` |
| 长短消息分段与合并 | 发送侧 `gsm_sms_sender.cpp` 与 `gsm/gsm_sms_message.cpp`；接收侧 `sms_receive_handler.cpp` |
| GSM 或 CDMA PDU 编解码 | `services/sms/gsm/`、`services/sms/cdma/` |
| 小区广播 | `gsm/gsm_sms_cb_handler.cpp`、`gsm_cb_*codec.cpp`、`sms_misc_manager.cpp` |
| 彩信下载或发送失败 | `services/mms/` |
| 彩信内容解析异常 | `frameworks/native/mms/src/` |
| 文本编码与字符集 | `utils/text_coder.cpp`、`frameworks/native/mms/src/utils/mms_charset.cpp` |
| 权限与系统应用限制 | `services/sms/sms_service.cpp` |

## 文件体量

下表为快照值，用于判断相对量级，不作为可校验断言。统计口径为含空行的行数。

| 文件 | 量级 |
|---|---|
| `interfaces/kits/js/@ohos.telephony.sms.d.ts` | ~3000 |
| `frameworks/js/napi/src/napi_sms.cpp` | ~2000 |
| `frameworks/native/mms/src/mms_header.cpp` | ~1700 |
| `frameworks/js/napi/src/napi_mms.cpp` | ~1700 |
| `services/sms/cdma/cdma_sms_sub_parameter.cpp` | ~1500 |
| `services/sms/sms_service.cpp` | ~1000 |
| `utils/text_coder.cpp` | ~1000 |
