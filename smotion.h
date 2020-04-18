#ifndef SMOTION_H_
#define SMOTION_H_
//#include "apt32f172.h"

#define uint8_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int

#define SM_RX_SIZE  1500
#define SM_UART_IDLE_TIM	5    //5ms


#define CMD_GPS_DATA    0x40
#define CMD_CAR_STANOW  0x41
#define CMD_CAR_STA1S   0x43
#define CMD_CAR_STA30S  0x42
//55
struct SmPack{
	uint16_t  head;//包头
	uint8_t   cmd;//命令
	uint8_t   pack_cnt;//包序号
	char      company_name[16];//公司名称
	uint8_t   UID[12];//模块UID
	uint8_t   SN[20];//SN号
	uint16_t  data_len;//数据长度
	//uint8_t data[1405];//数据
	uint8_t crc;//校验位
};
//------------------------------------------------------
#pragma pack(1)
struct GpsData{
	uint32_t time; //时间
	uint32_t longitude;//经度
	uint32_t latitude;//纬度
	uint32_t altitude;//海拔高度
	uint16_t speed; //gps速度
	uint16_t dir;   //gps方向
};//20
//0x40
struct SmPack_GpsData{
	uint16_t  head;
	uint8_t   cmd;
	uint8_t   pack_cnt;
	char      company_name[16];
	uint8_t   UID[12];
	uint8_t   SN[20];
	uint16_t  data_len;
	struct GpsData gpsData;
	uint8_t crc;
};

//------------------------------------------------------
struct CarStaNow{
	uint32_t time;//时间
	uint32_t time_stamp;//时间戳
	uint16_t coll_type;//碰撞类型
	short    event_thr;//阈值
	uint8_t  is_adjust;//是否校准
	struct GpsData gps_data;//gps数据
};//33
//0x41
struct SmPack_CarStaNow{
	uint16_t  head;
	uint8_t   cmd;
	uint8_t   pack_cnt;
	char      company_name[16];
	uint8_t   UID[12];
	uint8_t   SN[20];
	uint16_t  data_len;
	struct CarStaNow carStaNow;
	uint8_t crc;
};//88

//------------------------------------------------------
struct CarSta1S{
	uint32_t time;//时间
	uint32_t time_stamp;//时间戳
	uint16_t coll_type;//碰撞类型
	uint8_t gsensor_data[1200];//gsensor数据
};//1210
//0x43
struct SmPack_CarSta1S{
	uint16_t  head;
	uint8_t   cmd;
	uint8_t   pack_cnt;
	char      company_name[16];
	uint8_t   UID[12];
	uint8_t   SN[20];
	uint16_t  data_len;
	struct CarSta1S carSta1S;
	uint8_t crc;
};

//------------------------------------------------------
struct CarSta30S{
    uint32_t time;//时间
	uint32_t time_stamp;//时间戳
	uint16_t coll_type;//碰撞类型
	uint8_t  car_sta;//车辆状态
	struct GpsData gps_data;//gps数据
};//31
struct SmPack_CarSta30S{
	uint16_t  head;
	uint8_t   cmd;
	uint8_t   pack_cnt;
	char      company_name[16];
	uint8_t   UID[12];
	uint8_t   SN[20];
	uint16_t  data_len;
	struct CarSta30S carSta30S;
	uint8_t crc;
};
#pragma pack()



void sm_rx_data(uint8_t res);
void sm_uart_idle_handler(void);
void sm_data_parsing(void);
#endif

