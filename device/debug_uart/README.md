# 调试串口(debug_uart)

板级调试控制台:USART1,PA9(TX)/ PA10(RX),115200 8N1,无流控。

- **TX**:非阻塞环形缓冲(512 字节)+ DMA1 通道 4。`printf`/`DebugUart_Printf` 只做入队,立即返回,由 DMA 中断逐块排空发送;发送期间不占用 CPU。
- **RX**:DMA1 通道 5 循环缓冲(256 字节)+ 空闲线中断定界,对外提供带超时的阻塞读,适合按"帧"(如一行命令)接收变长数据。

## 接线

| 信号 | 引脚 | 说明 |
| --- | --- | --- |
| TX | PA9 | 接 USB-TTL 模块的 RXD |
| RX | PA10 | 接 USB-TTL 模块的 TXD |
| GND | GND | 与 USB-TTL 共地 |

3.3 V 供电的 USB-TTL 模块可直接连接;不要接 5 V 电平的模块。

## 输出:printf 与 DebugUart_Printf

`printf()` 已重定向到调试串口(实现了 `_write()`),直接使用即可:

```c
printf("count = %lu\n", (unsigned long)count);
```

多任务日志建议用 `DebugUart_Printf()`:它在互斥量保护下格式化并整行入队,日志行不会被其他任务打断;而 newlib 的 `printf()` 本身不可重入,并发调用可能在格式化阶段交错(入队后字节仍按到达顺序发送)。

```c
DebugUart_Printf("sensor: %d %d\n", a, b); /* '\n' 自动扩展为 "\r\n" */
```

两者都是非阻塞的:TX 环满时**整条消息丢弃**(不截断),累计字节数可用 `DebugUart_TxDropped()` 查询——非零说明有任务打印过快。

## 接收:阻塞读

```c
uint8_t buf[32];
uint16_t n = DebugUart_Read(buf, sizeof(buf), 100U); /* 超时 100 ms,0 = 超时无数据 */
if (n > 0U) {
    /* 处理 buf[0..n-1],空闲线中断保证一次读到的是一帧连续数据 */
}
```

- 空闲线中断(线路静默)将到目前为止收到的字节标记为一帧,一次 `DebugUart_Read` 倾向返回单帧内容;
- `timeout_ms` 传 `UINT32_MAX` 表示无限等待;
- `DebugUart_Available()` 查询当前缓冲字节数,可先判断再非阻塞地读;
- 若读取不及时,DMA 循环缓冲会覆盖最旧数据,`DebugUart_Read` 会自动跳到最新数据。

## 初始化与任务栈

`DebugUart_Init()` 必须在 `vTaskStartScheduler()` 之前调用(`main()` 已调用)。它会配置 GPIO、DMA、NVIC,创建互斥量/信号量并启动 RX。

在任务中调用 `DebugUart_Printf`/`printf` 时,任务栈建议不少于 **256 字(1 KB)**——newlib-nano 的 `vsnprintf` 栈开销远大于 `configMINIMAL_STACK_SIZE`(128 字)。`heartbeat_task` 已按此配置。

## 中断与资源占用

| 资源 | 用途 |
| --- | --- |
| DMA1 通道 4 | USART1 TX |
| DMA1 通道 5 | USART1 RX |
| DMA1 通道 4/5/USART1 中断 | 优先级 6(≥ `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`,允许在 ISR 中调 FreeRTOS `FromISR` API) |

⚠️ F103 的 DMA 通道与外设固定映射,**通道 4/5 与 SPI2、I2C2 的 DMA 互斥**;本模块占用后,这些外设只能用阻塞/中断方式(见 `driver/README.md` 的通道表)。

RAM 占用约 1.2 KB(TX 环 512 + RX 环 256 + 格式化缓冲 256 + DMA 句柄等)。
