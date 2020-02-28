KeyCarRosTest

com_tool node 负责串口通讯任务 
1 发送速度查询指令
2 获取速度返回指令
3 订阅CMD_VEL,解析并发送控制指令


key_vel_publisher 负责监听按键信息
1 处理按键信息 发送CMD_VEL