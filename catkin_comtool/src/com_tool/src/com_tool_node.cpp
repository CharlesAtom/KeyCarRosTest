#include <ros/ros.h>
#include <serial/serial.h> //ROS已经内置了的串口包
#include <std_msgs/String.h>
#include <std_msgs/Empty.h>
#include <std_msgs/UInt8MultiArray.h>
#include <std_msgs/MultiArrayDimension.h>
#include <geometry_msgs/Twist.h>

/****************************************************************************/
using std::string;
using std::exception;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
/*****************************************************************************/
serial::Serial ser; //声明串口对象


/**********************CmdVelAnalysis********************************************/
float ratio = 1000.0f ;   //转速转换比例，执行速度调整比例
float D = 0.2680859f ;    //两轮间距，单位是m
float linear_temp=0,angular_temp=0;//暂存的线速度和角速度
unsigned char data_terminal0=0x0d;  //“/r"字符
unsigned char data_terminal1=0x0a;  //“/n"字符
unsigned char speed_data[10]={0};   //要发给串口的数据


union floatData //union的作用为实现char数组和float之间的转换//发送给下位机的左右轮速度，里程计的坐标和方向
{
    float d;
    unsigned char data[4];
}right_speed_data,left_speed_data;//,position_x,position_y,oriention,vel_linear,vel_angular;
/*****************************************************************************/
/*****************************************************************************/

void CmdVelAnalysis(const geometry_msgs::Twist &cmd_vel)
{
    angular_temp = cmd_vel.angular.z ;//获取/cmd_vel的角速度,rad/s
    linear_temp = cmd_vel.linear.x ;//获取/cmd_vel的线速度.m/s

    //将转换好的小车速度分量为左右轮速度
    left_speed_data.d = linear_temp - 0.5f*angular_temp*D ;
    right_speed_data.d = linear_temp + 0.5f*angular_temp*D ;

    //存入数据到要发布的左右轮速度消息
    left_speed_data.d*=ratio;   //放大１０００倍，mm/s
    right_speed_data.d*=ratio;//放大１０００倍，mm/s

    for(int i=0;i<4;i++)    //将左右轮速度存入数组中发送给串口
    {
        speed_data[i]=right_speed_data.data[i];
        speed_data[i+4]=left_speed_data.data[i];
    }

    //在写入串口的左右轮速度数据后加入”/r/n“
    speed_data[8]=data_terminal0;
    speed_data[9]=data_terminal1;
    //写入数据到串口
    ser.write(speed_data,10);
}



//回调函数
void cmd_vel_callback(const geometry_msgs::Twist &cmd_vel)
{
    ROS_INFO("I heard linear velocity: x-[%f],y-[%f],", cmd_vel.linear.x, cmd_vel.linear.y);
    ROS_INFO("I heard angular velocity: [%f]", cmd_vel.angular.z);
    cout << "Twist Received" << endl;
    CmdVelAnalysis(cmd_vel);
}



int main(int argc, char **argv)
{
    //初始化节点
    ros::init(argc, argv, "serial_example_node");
    //声明节点句柄
    ros::NodeHandle nh;
    //订阅主题，并配置回调函数
    ros::Subscriber write_sub = nh.subscribe("/cmd_vel", 1000, cmd_vel_callback);
    //发布主题
    ros::Publisher read_pub = nh.advertise<std_msgs::String>("read", 1000);
    ros::Publisher chatter_pub = nh.advertise<std_msgs::UInt8MultiArray>("chatter", 1000);

    try
    {
        //设置串口属性，并打开串口
        ser.setPort("/dev/ttyUSB0");
        ser.setBaudrate(115200);
        serial::Timeout to = serial::Timeout::simpleTimeout(1000);
        ser.setTimeout(to);
        ser.open();
    }
    catch (serial::IOException &e)
    {
        ROS_ERROR_STREAM("Unable to open port ");
        return -1;
    }

    //检测串口是否已经打开，并给出提示信息
    if (ser.isOpen())
    {
        ROS_INFO_STREAM("Serial Port initialized");
    }
    else
    {
        return -1;
    }

    //指定循环的频率
    ros::Rate loop_rate(50);
    while (ros::ok())
    {

        // if(ser.available()){
        //     ROS_INFO_STREAM("Reading from serial port\n");
        //     std_msgs::String result;
        //     result.data = ser.read(ser.available());
        //     ROS_INFO_STREAM("Read: " << result.data);
        //     read_pub.publish(result);
        // }
        //获取缓冲区的字节数
        size_t n = ser.available();
        if (n != 0)
        {
            std_msgs::UInt8MultiArray m;

            // only needed if you don't want to use push_back
            m.data.resize(n);

            uint8_t buffer[1024];
            //读取数据
            n = ser.read(buffer, n);
            for (int i = 0; i < n; i++)
            {
                m.data[i] = buffer[i];
                std::cout << std::hex << (buffer[i] & 0xff) << " ";
            }
            std::cout << std::endl;

            chatter_pub.publish(m);

            //ser.write(buffer, n);
        }

        //处理ROS的信息，比如订阅消息,并调用回调函数
        ros::spinOnce();
        loop_rate.sleep();
    }
}