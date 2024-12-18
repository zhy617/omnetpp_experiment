### ARQ
需要实现的是停等 ARQ：发送完**一个**分组就停止发送，等待对方确认，收到确认后再发送下一个分组 

比较简单，只需要照抄word中的代码，并能正常运行即可。

实验检查时助教会问ARQ是什么？ARQ有什么特点？ARQ的劣势是什么？记得做好准备

至于结果也很容易在log中找到

### Aloha

Aloha的代码在sample中有，import即可。

注意sample里的代码并不是完整的Aloha，只是Aloha的一个不重传版本。

这个版本的Aloha在丢包时不会重传，而是直接跳过。

我个人认为这个版本的目的在于验证pure Aloha、Slotted Aloha的性能，即验证概率公式计算出来的结果。

#### 结果
首先得把每个config都完整跑一遍，finish()后会生成仿真数据。
仿真数据保存在 "aloha/results/***.vec" 中，双击打开后进入 Browse Data 界面，可以看到仿真数据。
![alt text](image.png)
其中channelUtilization是信道利用率，即信道被占用的时间占总时间的比例。这个理论上会表现出pure Aloha和Slotted Aloha的性能差异。
- pure Aloha的信道利用率为0.18左右
- Slotted Aloha的信道利用率为0.36左右

这里补充一下具体计算过程，以 Slotted Aloha 为例：
信道理由率的期望为 $E(N, p) = Np(1-p)^{N-1}$，将其对p求导，令导数为0，解出p，即为最优的p值。

$$
\begin{align*}
E' &= N((1-p)^{N-1} - p(N-1)(1-p)^{N-2}) \\
&= N(1-p)^{N-2}((1-p) - p(N-1)) \\
&= N(1-p)^{N-2}(1-Np) \\
\end{align*}
$$

所以 $N(1-p)^{N-2}(1-Np) = 0$，解出 $p = \frac{1}{N}$，代入原式得：
$$E = (1 - \frac{1}{N})^{N -1} = \frac{(1-\frac{1}{N})^{N}}{(1-\frac{1}{N})}$$

$\lim_{N \rightarrow \infin} E = \frac{1}{e}$

### CSMA
主要只修改了 Node.cc 中的 check_channel_busy 函数。

这里有几个细节

- data queue 长度太长，默认1000，改为3-5左右即可
- simTime单位是**s**，而不是ps，所以要注意时间的设置
- 传参请参考inet代码，用this->par("xxx")
- 参数设计不合理，CSMA-0 只要一个 node 抢占了信道，其他的 node 就很难抢到信道，所以要设置一个合理的 node 数量和 data queue 长度。
  - 实测 CSMA-1 和 CSMA-p 都可以node交替发送，只是需要等待较长时间才能抢占使用权。
  - CSMA-0 会出现一个 node 占用信道时间过长的情况，其他 node 很难抢占信道。（感觉是学长给的代码参数没调好。。。。）

#### 过程

- 1-persistentes CSMA：“节点需要持续监听信道，一旦节点发现信道空闲后，则立刻发送数据。”。
- 0-persistentes CSMA：“节点不连续监听信道，若该时刻节点监听信道为busy，那么等待一段时间后，再次进行监听。若节点该时刻监听信道为空闲，则立刻发送数据。”
- p-persistentes CSMA：“节点需要持续监听信道，一旦发现信道空闲后，节点以p的概率立刻发送数据，以1-p的概率不发送数据。若节点该时刻不发送数据，那么等待一段时间后，再次进行监听，并以p概率再次发送”。（注：这里所述的p概率可以理解成抛骰子赌大小，如果抛大，那么就发送，反之不发送。其中抛大的概率就是p，而抛小的概率就是1-p）
