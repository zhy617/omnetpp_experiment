### OMNETpp 
inet/transportlayer/tcp/flavours/TcpTahoeRenoFamily.cc 中修改ssthresh的值
[omnet官方文档](https://doc.omnetpp.org/inet/api-current/neddoc/index.html)

### TCP Tahoe

- 慢启动 slow_start： 每个MSS的ASK，cwnd += 1MSS，最后表现为2的次幂指数增长
- 拥塞避免 congestion avoidance： 每个MSS的ACK，cwnd += MSS/cwnd，最后表现为线性增长
- 超时重传 = 快速重传
- 快速重传 fast retransmit： 3个重复ACK，立即重传，
  - ssthresh = max(2, cwnd/2), cwnd = 1

### TCP Reno

- 慢启动 slow_start： 每个MSS的ASK，cwnd += 1MSS，最后表现为2的次幂指数增长

- 拥塞避免 congestion avoidance： 每个MSS的ACK，cwnd += mss * (mss/cwnd)，最后表现为线性增长

- 显式拥塞通知控制 ECN：需要双端都支持
  最小隔一个rtt，
  sshtresh = max(2mss, cwnd/2), cwnd = cwnd/2

- 快速重传：3个重复ACK，立即重传，
  - ssthresh = max(2mss, cwnd/2), cwnd = ssthresh + 3
  - 重传后进入快速恢复


- 快速恢复 fast recovery： 
  - 进入快速恢复：经由快速重传进入
  - 进行快速恢复：inflating 每收到一个重复ack，cwnd += mss，
    - 当窗口没超出recovery point，不发包
    - 当窗口超出recovery point，发包，同时窗口与rcv_wnd比较，取小者 (Eg DupACK 8)
  - 退出快速恢复：当unack > recovery point 代表丢包都已重传，退出快速恢复 deflating
    cwnd = sshtresh，恢复正常拥塞避免


- 超时重传：优先级最高！
  sshtresh = max(2mss, cwnd/2), cwnd = mss
  rto(retransmission timeout) = 2 * rtt

### TCP NewReno
#### 改进点
- Reno 在收到一个非冗余ack时就退出快速恢复，NewReno 在收到一个非冗余ack时，不退出快速恢复，继续快速恢复状态，直到unack > recovery point
- 当cwnd很小时，不能触发快速重传，只能通过超时重传

#### 过程

- 慢启动 slow_start： 每个MSS的ASK，cwnd += 1MSS，最后表现为2的次幂指数增长

- 拥塞避免 congestion avoidance： 每个MSS的ACK，cwnd += mss * (mss/cwnd)，最后表现为线性增长

- 快速重传：3个重复ACK，立即重传，
  - ssthresh = max(2mss, cwnd/2), cwnd = ssthresh + 3
  - 重传后进入快速恢复

- 快速恢复 fast recovery：
  - 进入快速恢复：经由快速重传进入
  - 进行快速恢复：
    - 每收到一个重复ack，inflating ：cwnd += mss，
      - 当窗口没超出recovery point，不发包
      - 当窗口超出recovery point，发包，同时窗口与rcv_wnd比较，取小者
    - 收到非冗余ack，deflating： cwnd减少ack部分
      - partial：不退出快速恢复，继续快速恢复状态，同时 inflating： cwnd += mss
      - full：退出快速恢复，恢复正常拥塞避免，cwnd = min(ssthresh, in_flight + mss)

- 超时重传：优先级最高！    
  sshtresh = max(2mss, cwnd/2), cwnd = mss


### TCP Westwood

#### 参数
- a 参数：控制slow start时，sshtresh的增长速度，a越大，sshtresh增长越慢
  - 当超时/冗余ack时，a += 0.25
  - 当cwnd >= sshtresh时，a = 1 ，避免影响拥塞避免

- bwe: band width estimate，估计带宽，通过采样带宽计算
- sample_bwe: 采样带宽，

平滑计算：$bwe_n = \frac{19}{21} bwe_{n-1} + \frac{1}{21}(sample\_bwe _n + sample\_bwe _{n-1})$

计算sshtresh：$sshtresh_n = bwe_n * RTT_{min} / a$


#### 过程
- 慢启动 slow_start： 每个MSS的ASK，cwnd += 1MSS，最后表现为2的次幂指数增长


- 拥塞避免 congestion avoidance： 每个MSS的ACK，cwnd += mss * (mss/cwnd)，最后表现为线性增长

- 更快速重传 faster retransmit ：3个重复ACK，立即重传，
  - $sshtresh_n = bwe_n * RTT_{min} / a$ ，
  - 重传后进入快速恢复

- 快速恢复 fast recovery：like Reno
  - 每收到一个重复ack，inflating ：cwnd += mss，
    - 当窗口没超出recovery point，不发包
    - 当窗口超出recovery point，发包，同时窗口与rcv_wnd比较，取小者 ?

### TCP Vegas

#### 参数
- BaseRTT: 基础RTT，取发送第一个包到收到第一个ACK的时间，即最小RTT
- Expected：期望的吞吐量 $= WindowSize / BaseRTT$
- Actual : 实际的吞吐量 $= WindowSize / RTT$
- double threshold: a b
  - Diff < a: 期望吞吐量和实际吞吐量差距较小，增大cwnd
  - Diff > b: 期望吞吐量和实际吞吐量差距较大，减小cwnd
  - a < Diff < b: 期望吞吐量和实际吞吐量差距适中，不变cwnd

#### 过程

- 超时重传：优先级最高！
  sshtresh = max(2mss, flight_size/2), cwnd = **2***mss

- 慢启动 slow_start： 每两个rtt判断一次diff，
  - 若diff > 2mss，代表cwnd增大过快，进入拥塞避免
  - 若diff < mss，正常慢启动

- 拥塞避免：
  - 若diff > 4mss，减小cwnd，但最小得为2mss
  - 若diff < 2mss，增大cwnd