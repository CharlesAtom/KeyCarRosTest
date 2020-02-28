#include <ros/ros.h>
#include <serial/serial.h> //ROS�Ѿ������˵Ĵ��ڰ�
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
serial::Serial ser; //�������ڶ���


/**********************CmdVelAnalysis********************************************/
float ratio = 1000.0f ;   //ת��ת��������ִ���ٶȵ�������
float D = 0.2680859f ;    //���ּ�࣬��λ��m
float linear_temp=0,angular_temp=0;//�ݴ�����ٶȺͽ��ٶ�
unsigned char data_terminal0=0x0d;  //��/r"�ַ�
unsigned char data_terminal1=0x0a;  //��/n"�ַ�
unsigned char speed_data[10]={0};   //Ҫ�������ڵ�����


union floatData //union������Ϊʵ��char�����float֮���ת��//���͸���λ�����������ٶȣ���̼Ƶ�����ͷ���
{
    float d;
    unsigned char data[4];
}right_speed_data,left_speed_data;//,position_x,position_y,oriention,vel_linear,vel_angular;
/*****************************************************************************/
/*****************************************************************************/

void CmdVelAnalysis(const geometry_msgs::Twist &cmd_vel)
{
    angular_temp = cmd_vel.angular.z ;//��ȡ/cmd_vel�Ľ��ٶ�,rad/s
    linear_temp = cmd_vel.linear.x ;//��ȡ/cmd_vel�����ٶ�.m/s

    //��ת���õ�С���ٶȷ���Ϊ�������ٶ�
    left_speed_data.d = linear_temp - 0.5f*angular_temp*D ;
    right_speed_data.d = linear_temp + 0.5f*angular_temp*D ;

    //�������ݵ�Ҫ�������������ٶ���Ϣ
    left_speed_data.d*=ratio;   //�Ŵ󣱣���������mm/s
    right_speed_data.d*=ratio;//�Ŵ󣱣���������mm/s

    for(int i=0;i<4;i++)    //���������ٶȴ��������з��͸�����
    {
        speed_data[i]=right_speed_data.data[i];
        speed_data[i+4]=left_speed_data.data[i];
    }

    //��д�봮�ڵ��������ٶ����ݺ���롱/r/n��
    speed_data[8]=data_terminal0;
    speed_data[9]=data_terminal1;
    //д�����ݵ�����
    ser.write(speed_data,10);
}



//�ص�����
void cmd_vel_callback(const geometry_msgs::Twist &cmd_vel)
{
    ROS_INFO("I heard linear velocity: x-[%f],y-[%f],", cmd_vel.linear.x, cmd_vel.linear.y);
    ROS_INFO("I heard angular velocity: [%f]", cmd_vel.angular.z);
    cout << "Twist Received" << endl;
    CmdVelAnalysis(cmd_vel);
}



int main(int argc, char **argv)
{
    //��ʼ���ڵ�
    ros::init(argc, argv, "serial_example_node");
    //�����ڵ���
    ros::NodeHandle nh;
    //�������⣬�����ûص�����
    ros::Subscriber write_sub = nh.subscribe("/cmd_vel", 1000, cmd_vel_callback);
    //��������
    ros::Publisher read_pub = nh.advertise<std_msgs::String>("read", 1000);
    ros::Publisher chatter_pub = nh.advertise<std_msgs::UInt8MultiArray>("chatter", 1000);

    try
    {
        //���ô������ԣ����򿪴���
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

    //��⴮���Ƿ��Ѿ��򿪣���������ʾ��Ϣ
    if (ser.isOpen())
    {
        ROS_INFO_STREAM("Serial Port initialized");
    }
    else
    {
        return -1;
    }

    //ָ��ѭ����Ƶ��
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
        //��ȡ���������ֽ���
        size_t n = ser.available();
        if (n != 0)
        {
            std_msgs::UInt8MultiArray m;

            // only needed if you don't want to use push_back
            m.data.resize(n);

            uint8_t buffer[1024];
            //��ȡ����
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

        //����ROS����Ϣ�����綩����Ϣ,�����ûص�����
        ros::spinOnce();
        loop_rate.sleep();
    }
}