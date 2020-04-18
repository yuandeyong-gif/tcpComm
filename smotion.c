#include "smotion.h"
#include <stdio.h>

#define my_printf printf

/*************************uart rx driver start***********************************/

uint8_t sm_rx_idle_tim = 0,sm_rx_irq = 0,sm_rx_flag = 1;

uint16_t sm_rx_len = 88,sm_rx_len_tmp = 0;//记录接收数据长度

uint8_t sm_rx_buff[SM_RX_SIZE] = { 0x47,0x4d,//head
                                   0x41,//cmd
                                   0x4,//pack num
				   0x0,0x0,0x0,0x0,0x0,0x0,0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0,0x0,0x0, //company char[16]
                                   0x23,0x0,0xe,0x0,0x3,0x43,0x4e,0x48,0x30,0x36,0x34,0x20,  //UID u8[12]
				   0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,  //SN u8[16]
                                   0x21,0x0,   //data len u16
                                   0x0,0x0,0x0,0x0,//time u32
				   0x47,0x72,0x1,0x0,//timestamp u32
				   0x3,0x0, //coll type u16
				   0xfe,0x6,//coll thr  s16
                                   0x0,//is adjust
                                   0x0,0x0,0x0,0x0,//UTC time
                                   0x0,0x0,0x0,0x0,//longitude  u32
                                   0x0,0x0,0x0,0x0,//latitude  u32
                                   0x0,0x0,0x0,0x0,//altitude  u32
				   0x0,0x0,//speed u16
				   0x0,0x0,//dir
                                   0x30 //crc
       };

/*
 * 功能：串口接收数据
 * 参数：中断中接收的一个字节数据
 * 返回：无
 * */
void sm_rx_data(uint8_t res)
{
	//数据接收超过最大值或者数据未处理完不接受新的数据
	if((sm_rx_len_tmp >= SM_RX_SIZE) || (sm_rx_flag == 1))
	{
		return;
	}

    sm_rx_buff[sm_rx_len_tmp] = res;//保存数据
	sm_rx_len_tmp ++;
	sm_rx_idle_tim = 0;//串口空闲计时清零
	sm_rx_irq = 1;//置1表示有串口新数据
}
/*
 * 功能：串口空闲判断，该函数放到1ms定时器中断中
 * 参数：无
 * 返回：无
 * */
void sm_uart_idle_handler(void)
{
	sm_rx_idle_tim ++;
	if(sm_rx_irq && (sm_rx_idle_tim>SM_UART_IDLE_TIM)) //连续5ms未接收到数据，串口空闲
	{
		sm_rx_len = sm_rx_len_tmp;
		sm_rx_len_tmp = 0;
		sm_rx_flag = 1;//一帧数据接收完成，标志位置一
		sm_rx_idle_tim = 0;
		sm_rx_irq = 0;
	}
}

/*************************uart rx driver end***********************************/

/******************************* crc ***********************************************/
static const unsigned char crc_table[] =
{
    0x00,0x31,0x62,0x53,0xc4,0xf5,0xa6,0x97,0xb9,0x88,0xdb,0xea,0x7d,0x4c,0x1f,0x2e,
    0x43,0x72,0x21,0x10,0x87,0xb6,0xe5,0xd4,0xfa,0xcb,0x98,0xa9,0x3e,0x0f,0x5c,0x6d,
    0x86,0xb7,0xe4,0xd5,0x42,0x73,0x20,0x11,0x3f,0x0e,0x5d,0x6c,0xfb,0xca,0x99,0xa8,
    0xc5,0xf4,0xa7,0x96,0x01,0x30,0x63,0x52,0x7c,0x4d,0x1e,0x2f,0xb8,0x89,0xda,0xeb,
    0x3d,0x0c,0x5f,0x6e,0xf9,0xc8,0x9b,0xaa,0x84,0xb5,0xe6,0xd7,0x40,0x71,0x22,0x13,
    0x7e,0x4f,0x1c,0x2d,0xba,0x8b,0xd8,0xe9,0xc7,0xf6,0xa5,0x94,0x03,0x32,0x61,0x50,
    0xbb,0x8a,0xd9,0xe8,0x7f,0x4e,0x1d,0x2c,0x02,0x33,0x60,0x51,0xc6,0xf7,0xa4,0x95,
    0xf8,0xc9,0x9a,0xab,0x3c,0x0d,0x5e,0x6f,0x41,0x70,0x23,0x12,0x85,0xb4,0xe7,0xd6,
    0x7a,0x4b,0x18,0x29,0xbe,0x8f,0xdc,0xed,0xc3,0xf2,0xa1,0x90,0x07,0x36,0x65,0x54,
    0x39,0x08,0x5b,0x6a,0xfd,0xcc,0x9f,0xae,0x80,0xb1,0xe2,0xd3,0x44,0x75,0x26,0x17,
    0xfc,0xcd,0x9e,0xaf,0x38,0x09,0x5a,0x6b,0x45,0x74,0x27,0x16,0x81,0xb0,0xe3,0xd2,
    0xbf,0x8e,0xdd,0xec,0x7b,0x4a,0x19,0x28,0x06,0x37,0x64,0x55,0xc2,0xf3,0xa0,0x91,
    0x47,0x76,0x25,0x14,0x83,0xb2,0xe1,0xd0,0xfe,0xcf,0x9c,0xad,0x3a,0x0b,0x58,0x69,
    0x04,0x35,0x66,0x57,0xc0,0xf1,0xa2,0x93,0xbd,0x8c,0xdf,0xee,0x79,0x48,0x1b,0x2a,
    0xc1,0xf0,0xa3,0x92,0x05,0x34,0x67,0x56,0x78,0x49,0x1a,0x2b,0xbc,0x8d,0xde,0xef,
    0x82,0xb3,0xe0,0xd1,0x46,0x77,0x24,0x15,0x3b,0x0a,0x59,0x68,0xff,0xce,0x9d,0xac
}; 

unsigned char cal_crc_table(unsigned char *ptr, uint16_t len) 
{
    unsigned char  crc = 0x00;

    while (len--)
    {
        crc = crc_table[crc ^ *ptr++];
    }
    return (crc);
}

/*
 *功能：校验数据有效性
 *参数： rx_data:s-motion模块串口数据buff头指针   len：数据长度
 *返回： 0：校验成功  1：校验失败 2:包头不正确
 */
uint8_t sm_check_crc(uint8_t *rx_data,uint16_t len)
{
	uint16_t crc_len = len -1;
	uint8_t rx_crc = rx_data[len - 1];
	if(rx_data[0] != 0x47 || rx_data[1] != 0x4d)
	{
		return 2;
	}
	if(cal_crc_table(rx_data,crc_len) == rx_crc)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
/******************************* crc end *******************************************/


void logCAR_SATNOW(struct SmPack_CarStaNow *pSmPack_CarStaNow)
{
#if 0
	uint16_t head;
	uint8_t cmd ;
	head = pSmPack_CarStaNow->head;
	cmd = pSmPack_CarStaNow->cmd;
	//my_printf("pSmPack_CarStaNow test\r\n");
	my_printf("head=%x\r\n",head);
	my_printf("cmd=%x\r\n",cmd);
#endif
	my_printf("head=%x\r\n",pSmPack_CarStaNow->head);
	my_printf("cmd=%x\r\n",pSmPack_CarStaNow->cmd);
	my_printf("pack_cnt=%x\r\n",pSmPack_CarStaNow->pack_cnt);
	my_printf("company=: ");
	for(int i=0;i<sizeof(pSmPack_CarStaNow->company_name);i++)
	{
		my_printf("%x ",pSmPack_CarStaNow->company_name[i]);
	}
	my_printf("\r\n");
    my_printf("UID=: ");
	for(int i=0;i<sizeof(pSmPack_CarStaNow->UID);i++)
	{
		my_printf("%x ",pSmPack_CarStaNow->UID[i]);
	}
	my_printf("\r\n");
    my_printf("SN=: ");
	for(int i=0;i<sizeof(pSmPack_CarStaNow->SN);i++)
	{
		my_printf("%x ",pSmPack_CarStaNow->SN[i]);
	}
	my_printf("\r\n");
	my_printf("data len=%x\r\n",pSmPack_CarStaNow->data_len);
	//data
	my_printf("time=%x\r\n",pSmPack_CarStaNow->carStaNow.time);
	my_printf("timestamp=%x\r\n",pSmPack_CarStaNow->carStaNow.time_stamp);
	my_printf("coll_type=%x\r\n",pSmPack_CarStaNow->carStaNow.coll_type);
	my_printf("event_thr=%x\r\n",pSmPack_CarStaNow->carStaNow.event_thr);
	my_printf("is_adjust=%x\r\n",pSmPack_CarStaNow->carStaNow.is_adjust);
	//gps data
	my_printf("gps time=%x\r\n",pSmPack_CarStaNow->carStaNow.gps_data.time);
	my_printf("gps longitude=%x\r\n",pSmPack_CarStaNow->carStaNow.gps_data.longitude);
	my_printf("gps latitude=%x\r\n",pSmPack_CarStaNow->carStaNow.gps_data.latitude);
	my_printf("gps altitude=%x\r\n",pSmPack_CarStaNow->carStaNow.gps_data.altitude);
	my_printf("gps speed=%x\r\n",pSmPack_CarStaNow->carStaNow.gps_data.speed);
	my_printf("gps dir=%x\r\n",pSmPack_CarStaNow->carStaNow.gps_data.dir);
	//crc
	my_printf("crc=%x\r\n",pSmPack_CarStaNow->crc);
#if 0	
	//test
	pSmPack_CarStaNow->data_len = 0x9988;
	my_printf("data len=%x\r\n",pSmPack_CarStaNow->data_len);
	my_printf("data len b=%x %x\r\n",sm_rx_buff[52],sm_rx_buff[53]);
#endif
}

void doCMD_CAR_STANOW(uint8_t *rx_data,uint16_t len)
{
	struct SmPack_CarStaNow *pSmPack_CarStaNow;
	my_printf("doCMD_CAR_SATNOW\r\n");
	pSmPack_CarStaNow = (struct SmPack_CarStaNow *)rx_data;
	//log
	logCAR_SATNOW(pSmPack_CarStaNow);
}
//----------------------------------------------------------------------------------------------------
void logCMD_CAR_SAT1S(struct SmPack_CarSta1S *pSmPack_CarSta1S)
{
	my_printf("head=%x\r\n",pSmPack_CarSta1S->head);
	my_printf("cmd=%x\r\n",pSmPack_CarSta1S->cmd);
	my_printf("pack_cnt=%x\r\n",pSmPack_CarSta1S->pack_cnt);
	my_printf("company=: ");
	for(int i=0;i<sizeof(pSmPack_CarSta1S->company_name);i++)
	{
		my_printf("%x ",pSmPack_CarSta1S->company_name[i]);
	}
	my_printf("\r\n");
    my_printf("UID=: ");
	for(int i=0;i<sizeof(pSmPack_CarSta1S->UID);i++)
	{
		my_printf("%x ",pSmPack_CarSta1S->UID[i]);
	}
	my_printf("\r\n");
    my_printf("SN=: ");
	for(int i=0;i<sizeof(pSmPack_CarSta1S->SN);i++)
	{
		my_printf("%x ",pSmPack_CarSta1S->SN[i]);
	}
	my_printf("\r\n");
	my_printf("data len=%x\r\n",pSmPack_CarSta1S->data_len);
	//data
	my_printf("time=%x\r\n",pSmPack_CarSta1S->carSta1S.time);
	my_printf("timestamp=%x\r\n",pSmPack_CarSta1S->carSta1S.time_stamp);
	my_printf("coll_type=%x\r\n",pSmPack_CarSta1S->carSta1S.coll_type);
	//gsensor data
	my_printf("gsnsor data: ");
	for(int i=0;i<sizeof(pSmPack_CarSta1S->carSta1S.gsensor_data);i++)
	{
		my_printf("%x ",pSmPack_CarSta1S->carSta1S.gsensor_data[i]);
	}
	my_printf("\r\n");
	//crc
	my_printf("crc=%x\r\n",pSmPack_CarSta1S->crc);
}

void doCMD_CAR_STA1S(uint8_t *rx_data,uint16_t len)
{
	struct SmPack_CarSta1S *pSmPack_CarSta1S;
	my_printf("doCMD_CAR_SAT1S\r\n");
	pSmPack_CarSta1S = (struct SmPack_CarSta1S *)rx_data;
	//log
	logCMD_CAR_SAT1S(pSmPack_CarSta1S);
}

//----------------------------------------------------------------------------------------------------
void logCMD_CAR_SAT30S(struct SmPack_CarSta30S *pSmPack_CarSta30S)
{
	my_printf("head=%x\r\n",pSmPack_CarSta30S->head);
	my_printf("cmd=%x\r\n",pSmPack_CarSta30S->cmd);
	my_printf("pack_cnt=%x\r\n",pSmPack_CarSta30S->pack_cnt);
	my_printf("company=: ");
	for(int i=0;i<sizeof(pSmPack_CarSta30S->company_name);i++)
	{
		my_printf("%x ",pSmPack_CarSta30S->company_name[i]);
	}
	my_printf("\r\n");
    my_printf("UID=: ");
	for(int i=0;i<sizeof(pSmPack_CarSta30S->UID);i++)
	{
		my_printf("%x ",pSmPack_CarSta30S->UID[i]);
	}
	my_printf("\r\n");
    my_printf("SN=: ");
	for(int i=0;i<sizeof(pSmPack_CarSta30S->SN);i++)
	{
		my_printf("%x ",pSmPack_CarSta30S->SN[i]);
	}
	my_printf("\r\n");
	my_printf("data len=%x\r\n",pSmPack_CarSta30S->data_len);
	//data
	my_printf("time=%x\r\n",pSmPack_CarSta30S->carSta30S.time);
	my_printf("timestamp=%x\r\n",pSmPack_CarSta30S->carSta30S.time_stamp);
	my_printf("coll_type=%x\r\n",pSmPack_CarSta30S->carSta30S.coll_type);
    my_printf("car sta=%x\r\n",pSmPack_CarSta30S->carSta30S.car_sta);
	//gps data
	my_printf("gps time=%x\r\n",pSmPack_CarSta30S->carSta30S.gps_data.time);
	my_printf("gps longitude=%x\r\n",pSmPack_CarSta30S->carSta30S.gps_data.longitude);
	my_printf("gps latitude=%x\r\n",pSmPack_CarSta30S->carSta30S.gps_data.latitude);
	my_printf("gps altitude=%x\r\n",pSmPack_CarSta30S->carSta30S.gps_data.altitude);
	my_printf("gps speed=%x\r\n",pSmPack_CarSta30S->carSta30S.gps_data.speed);
	my_printf("gps dir=%x\r\n",pSmPack_CarSta30S->carSta30S.gps_data.dir);
	//crc
	my_printf("crc=%x\r\n",pSmPack_CarSta30S->crc);
}

void doCMD_CAR_STA30S(uint8_t *rx_data,uint16_t len)
{
	struct SmPack_CarSta30S *pSmPack_CarSta30S;
	pSmPack_CarSta30S = (struct SmPack_CarSta30S *)rx_data;
	my_printf("doCMD_CAR_SAT30S\r\n");
	//log
	logCMD_CAR_SAT30S(pSmPack_CarSta30S);
}
//----------------------------------------------------------------------------------------------------

void do_cmd(uint8_t *rx_data,uint16_t len)
{
	uint8_t cmd = 0;
	cmd = rx_data[2];
	switch(cmd)
	{
		case CMD_CAR_STANOW:
			doCMD_CAR_STANOW(rx_data,len);
		break;
		case CMD_CAR_STA1S:
			doCMD_CAR_STA1S(rx_data,len);
		break;
		case CMD_CAR_STA30S:
			doCMD_CAR_STA30S(rx_data,len);
		break;
		default:
			my_printf("not support this cmd\r\n");
		break;
	}
}

void sm_data_parsing(void)
{
	if(sm_rx_flag == 1)
	{
		uint8_t ret = 0;
		my_printf("smoion data:\r\n");
		for(int i=0;i<sm_rx_len;i++)
		{
			printf("%2x ",sm_rx_buff[i]);
		}
		my_printf("\r\n");
		ret = sm_check_crc(sm_rx_buff,sm_rx_len);//校验
		if(0 != ret)
		{
			my_printf("crc err sta=%d\r\n",ret);
		}
		else
		{
			my_printf("crc success\r\n");
			do_cmd(sm_rx_buff,sm_rx_len);
		}
		
		sm_rx_flag = 0;
	}
}

int main(void)
{
  sm_data_parsing();
}









