# 运维观测

← 返回 [`AGENTS.md`](../../AGENTS.md)

---

## 日志标识

| 项 | 值 | 定义位置 |
|---|---|---|
| 服务日志标签 | `SmsMms` | 根目录 `BUILD.gn` 的 `defines` |
| 服务日志域 | `0xD001F06` | 根目录 `BUILD.gn` |
| 进程 | `telephony` | `sa_profile/4008.json` |
| 系统服务 | 4008 | 同上 |

```shell
hdc shell hilog -D 0xD001F06
hdc shell hilog | grep -i SmsMms
```

彩信编解码库在应用进程中运行，解码相关日志出现在调用方应用的进程日志里，不在服务进程日志里。

## dump

```shell
hdc shell hidumper -s 4008 -a "-sms_mms_info"
```

`services/sms/sms_dump_helper.cpp` 支持 `-sms_mms_info`、`-output_slot`、`-output_service_state`，输出服务绑定时间、耗时、默认卡槽与服务状态。

## 关键日志

### 发送

| 日志片段 | 位置 | 判读 |
|---|---|---|
| `netWorkType =` | `SmsSendManager::TextBasedSmsDelivery` | 本次发送选择的网络制式 |
| `network unknown send error.` | 同上 | 网络制式未知，直接失败 |
| `SendSms not in service` | `GsmSmsSender::SendSmsToRil` | 非 IMS 域且语音服务不在服务状态 |
| `gsm refId =` | 同上 | 本次发送的引用号，用于与响应对齐 |
| `SendCacheMapAddItem Error` | 同上 | 发送缓存已满或写入失败 |

### 接收

| 日志片段 | 位置 | 判读 |
|---|---|---|
| `Received a new message and pushed it in queue` | `HandleReceivedSmsWithoutDataShare` | DataShare 未就绪，消息已确认并入队 |
| `The multi-page text didn't all arrive` | `CombineMessagePart` | 长短消息分段未到齐，等待后续分段 |

### 彩信

| 日志片段 | 位置 | 判读 |
|---|---|---|
| `download mms successed` / `download mms failed` | `MmsReceiveManager::DownloadMms` | 下载事务结果 |
| `Decode Mms Header Error. Error Location:` | `MmsMsg::DecodeMsg` | 头部解码中止，数字为解码缓冲区偏移 |
| `The fieldId[...] decode header fail.` | `MmsHeader::DecodeMmsHeader` | 指出是哪个字段的解码函数返回失败 |
| `DecodeMmsMsgUnKnownField:` | 同上 | 遇到未登记的字段码，按未知字段跳过 |
| `Decode encodeString ... fail.` | `MmsEncodeString::DecodeEncodeString` | 编码字符串字段解码失败，需结合其返回值判断是否中止整体解码 |

## HiSysEvent

`utils/sms_hisysevent.h` 提供四个入口：`WriteSmsSendBehaviorEvent`、`WriteSmsReceiveBehaviorEvent`、`WriteSmsSendFaultEvent`、`WriteSmsReceiveFaultEvent`。

故障类打点携带 `SmsMmsErrorCode`，例如 `SMS_ERROR_UNKNOWN_NETWORK_TYPE`、`SMS_ERROR_SENDSMS_NOT_IN_SERVICE`。新增失败分支时应补故障打点。

## 公共事件

| 事件 | 触发 |
|---|---|
| `COMMON_EVENT_SMS_RECEIVE_COMPLETED` | 普通短消息接收完成 |
| `COMMON_EVENT_SMS_WAPPUSH_RECEIVE_COMPLETED` | WAP Push 接收完成 |
| `COMMON_EVENT_SMS_CB_RECEIVE_COMPLETED` | 小区广播接收完成 |
| `COMMON_EVENT_SMS_EMERGENCY_CB_RECEIVE_COMPLETED` | 紧急小区广播接收完成 |

## 排查顺序

1. 发送问题先看 `netWorkType` 与 `SendSmsToRil` 相关日志，确认网络制式与发送域。
2. 接收问题先确认 DataShare 是否就绪，再看分段是否到齐与公共事件是否发布。
3. 彩信下载问题看服务进程的 HTTP 与数据网络日志；彩信内容问题看应用进程的解码日志。
4. 定位到模块后，转到 [`failure-modes.md`](failure-modes.md) 的「快速分诊表」。
