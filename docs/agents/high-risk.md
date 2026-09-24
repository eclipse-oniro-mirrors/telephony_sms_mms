# 高危入口与变更风险

← 返回 [`AGENTS.md`](../../AGENTS.md)

---

## 高危入口

### 等级 A

| 入口 | 位置 | 风险 |
|---|---|---|
| 紧急小区广播 | `gsm/gsm_sms_cb_handler.cpp`、`gsm_cb_*codec.cpp`、`COMMON_EVENT_SMS_EMERGENCY_CB_RECEIVE_COMPLETED` | 预警消息丢失或错误展示 |
| 网络侧 PDU 解码 | `services/sms/gsm/`、`services/sms/cdma/`、`sms_wap_push_handler.cpp` | 恶意 PDU 触发越界读写，服务进程崩溃 |
| 彩信编解码 | `frameworks/native/mms/src/` | 同时运行在应用与服务进程；解码过严导致正常彩信无法展示，过松导致越界 |
| 彩信 HTTP 事务 | `services/mms/mms_network_client.cpp`、`mms_receive.cpp`、`mms_sender.cpp` | 响应大小、地址校验的改动影响安全边界 |
| 权限收口 | `services/sms/sms_service.cpp` | 放宽即越权收发短消息 |
| FFI 与 NAPI 入口 | `frameworks/cj/`、`frameworks/js/napi/`、`frameworks/ets/taihe/` | 空指针与长度参数直接来自应用 |

### 等级 B

| 入口 | 位置 | 风险 |
|---|---|---|
| 长短消息拆分与合并 | `gsm_sms_sender.cpp`、`sms_receive_handler.cpp` | 长消息发送失败、接收后无法合并、分段记录残留 |
| 接收入队与 DataShare 就绪 | `SmsReceiveHandler` | 开机阶段消息丢失 |
| 发送结果回调 | `SmsSender`、`SmsSendManager` | 应用侧一直等待发送结果 |
| 发送域选择 | `SmsSender`、`SmsNetworkPolicyManager` | IMS 与 CS 选择错误导致发送失败 |
| 短码分类 | `sms_short_code_matcher.cpp`、`services/etc/sms_short_code_rules.json` | 付费短消息提示失效 |
| 文本编码 | `utils/text_coder.cpp` | 字符集转换错误，正文乱码 |

---

## 高危配置项

| `smsmms.gni` 变量 | 默认 | 宏或影响 |
|---|---|---|
| `sms_mms_dynamic_start` | `false` | 使用 `4008_dynamic.json` 按需启停 |
| `sms_mms_tel_power_mode` | `false` | `BASE_POWER_IMPROVEMENT_FEATURE` |
| `sms_mms_feature_support_mms` | `true` | 彩信服务源文件与 `SMS_SUPPORT_MMS` |
| `sms_mms_satellite` | `false` | `SMS_MMS_SATELLITE`，卫星短消息源文件与 `sms_satellite_gtest.cpp` |

由外部部件推导的宏：`ABILITY_POWER_SUPPORT`（power_manager 存在）、`OHOS_BUILD_ENABLE_TELEPHONY_EXT`（增强特性打开）。

新增 feature 宏清单：`smsmms.gni` 的 `declare_args`、`bundle.json` 的 `features`、根目录 `BUILD.gn` 条件块、`test/sms_mms_test.gni` 的 `sms_mms_defines` 与条件源文件。宏关闭时必须能编过。

---

## 高危事件序列

| 序列 | 关注点 |
|---|---|
| 开机，DataShare 未就绪时收到短消息 | 确认、入队、就绪后补处理，全程不丢 |
| 长短消息分段乱序、部分丢失、重复到达 | 合并判定、段序号校验、残留记录清理 |
| 发送中 RIL 进程重启 | `OnRilAdapterHostDied` 对缓存中请求回调失败 |
| 发送中 IMS 注册状态变化 | 发送域切换与重试 |
| 彩信下载中数据网络断开 | 事务超时、网络释放、结果回调 |
| 双卡同时收发 | 每卡槽独立管理器，共享数据库 |

---

## 变更风险矩阵

| 改动模块 | 回归面 | 最低验证 |
|---|---|---|
| `frameworks/native/mms/` 解码 | 全部彩信类型的解析、应用侧 `decodeMms` | `unittest_mms_gtest`、`unittest_zero_branch_mms_test`，附最小字节序列用例 |
| `frameworks/native/mms/` 编码 | 彩信发送、应用侧 `encodeMms` | 同上 |
| `services/mms/` | 彩信下载与发送 | `unittest_sms_services_mms_gtest`、`unittest_mms_receiver_mgr_gtest` |
| `services/sms/gsm/` 编解码与发送 | GSM 收发、长短消息 | `unittest_sms_gsm_gtest`、`unittest_zero_branch_sms_test` |
| `services/sms/cdma/` | CDMA 收发 | `unittest_cdma_sms_gtest`、`unittest_zero_branch_cdma_sms_test` |
| `sms_receive_handler.cpp` | 接收、合并、入队 | `tel_sms_mms_gtest`（源文件 `zero_branch_sms_part_test.cpp`，合并用例集中于此）、`unittest_zero_branch_sms_test`、`unittest_sms_mms_receive_gtest` |
| 小区广播 | 普通与紧急广播 | `unittest_cb_gtest`、`unittest_zero_branch_cb_test` |
| `utils/` | 全仓编解码与打点 | `unittest_sms_utils_gtest`、`unittest_zero_branch_utils_test` |
| `sms_service.cpp` 权限 | 全部接口 | 搜索 `test/gtest/` 中调用被改接口的用例，确认其所在目标并运行；无权限替身，权限拒绝路径在最终回复中说明验证方式 |
| 前端 | 三套前端对应接口 | 无单测，静态自检加设备验证 |
| feature 宏 | 宏打开与关闭两种构建 | 两种配置各编译一次 |
