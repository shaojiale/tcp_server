# tcpclient V1.6
# 加入C++定时器 测试每秒包数量
# 修复1.5bug 目前大概支持5000连接
# tcpclient V1.5
# 加入接受缓冲区和消息缓冲区测试性能最高达到12Gbps，正常3-7Gbps
# 客户端创建两个socket能够正常连接，从第三个起需要保持两个客户端运行才能正常收发
# 可能是客户端发送堵塞
# tcp_server V1.4
# 修复BUG
# 在读fd循环中，循环上下是select可读的socket不是客户端的范围
# tcp_server V1.3
# 封装成类 
# 不能多个客户端连接
# tcp_server V1.2
# 学习用
# 加入select多路I/O服用模型
# 格式化消息
# 简易接受消息回发数据