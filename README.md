# Marmota

告警采集转发系统，告警等级与zlog日志等级对齐，支持限频和多路径导出。

前提：告警信息小于日志数量，基本只有Fatal/Error/Warn级别。

```txt
                         [AlarmFatal()]
                                |
                                | tryPush
                                |
        [Multi-Thread in and Single-Thread out queue...]
                                |
                                | tryPop in another thread
                                |
                        [Interceptor-List]
                                |
                                |
                         [Exporter-List]
```
