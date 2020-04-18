/**
* @file    hi_gpsmng.c
* @brief   product gpsmng function
*
* Copyright (c) 2017 Huawei Tech.Co.,Ltd
*
* @author    Huawei team
* @date      2018/12/11
* @version

*/
#include <string.h>
#include <sys/prctl.h>
#include <pthread.h>
#include "hi_gpsmng.h"
#include "gpsmng_analysis.h"
#include "hi_hal_gps.h"
#include "hi_hal_common.h"
#include "hi_appcomm_util.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif  /* End of #ifdef __cplusplus */

#define GPSMNG_DEBUG
#define GPSMNG_GET_DATA_TIMEOUT_MS 300

/*  config for user,such as: config bit0 is 1 Represents the need to have gga raw data before it can be parsed
 *  GPSMNG_RAW_DATA_MASK:|bit7 -- bit6 -- bit5 -- bit4 -- bit3 -- bit2 -- bit1 -- bit0|
 *                                       GPGSV   GPVTG   GPRMC   GPGSA   GPGLL   GPGGA
 */

#define GPSMNG_RAW_DATA_MASK 0X1f /* contain GPVTG && GPRMC && GPGSA && GPGLL && GPGGA */


typedef enum  tagGPSMNG_STATE
{
    GPSMNG_STATE_DEINITED,
    GPSMNG_STATE_INITED,
    GPSMNG_STATE_STARTED,
    GPSMNG_STATE_STOP,
    GPSMNG_STATE_BUTT,
} GPSMNG_STATE;

typedef struct tagGPSMNG_CONTEXT
{
    HI_BOOL gpsReadFlg;
    HI_BOOL gpsRefreshData;
    pthread_t gpsReadThd;
    pthread_mutex_t gpsLock;
    GPSMNG_STATE gpsMngState;
    HI_GPSMNG_CALLBACK fnGpsCB[HI_GPSMNG_CALLBACK_MAX_NUM];
} GPSMNG_CONTEXT;

typedef struct tagGPSMNG_RAWDATA_BUFF
{
    HI_S32 msgCount;
    HI_S32 gpsGPGSVMsgTotalNum;
    HI_S32 gpsGPGSVMsgCurNum;
    GPSMNG_RAW_DATA gpsRawData;
} GPSMNG_RAWDATA_BUFF;

static GPSMNG_CONTEXT g_gpsMngCtx;
static GPSMNG_RAWDATA_BUFF g_gpsRawdataBuff;
static HI_GPSMNG_MSG_PACKET g_gpsMngPacket;

/* Refresh a complete set of gps data to buf */
static HI_S32 GPSMNG_MsgProc(HI_CHAR* msgStr)
{
    if (msgStr == NULL)
    {
        MLOGW("Gps Message String is null\n");
        return HI_FAILURE;
    }

    if (HI_NULL != strstr(msgStr, "GPGGA"))
    {
        snprintf(g_gpsRawdataBuff.gpsRawData.ggaStr, HI_GPSMNG_MESSAGE_MAX_LEN, "%s", msgStr);
        g_gpsRawdataBuff.msgCount |= 1 << 0;
    }
    else if (HI_NULL != strstr(msgStr, "GPGLL"))
    {
        snprintf(g_gpsRawdataBuff.gpsRawData.gllStr, HI_GPSMNG_MESSAGE_MAX_LEN, "%s", msgStr);
        g_gpsRawdataBuff.msgCount |= 1 << 1;
    }
    else if (HI_NULL != strstr(msgStr, "GPGSA"))
    {
        snprintf(g_gpsRawdataBuff.gpsRawData.gsaStr, HI_GPSMNG_MESSAGE_MAX_LEN, "%s", msgStr);
        g_gpsRawdataBuff.msgCount |= 1 << 2;
    }
    else if (HI_NULL != strstr(msgStr, "GPRMC"))
    {
        snprintf(g_gpsRawdataBuff.gpsRawData.rmcStr, HI_GPSMNG_MESSAGE_MAX_LEN, "%s", msgStr);
        g_gpsRawdataBuff.msgCount |= 1 << 3;
    }
    else if (HI_NULL != strstr(msgStr, "GPVTG"))
    {
        snprintf(g_gpsRawdataBuff.gpsRawData.vtgStr, HI_GPSMNG_MESSAGE_MAX_LEN, "%s", msgStr);
        g_gpsRawdataBuff.msgCount |= 1 << 4;
    }
    else if (HI_NULL != strstr(msgStr, "GPGSV"))
    {
        HI_S32 totalMsgNum = 0;
        HI_S32 curMsgNum = 0;
        sscanf(msgStr, "$GPGSV,%d,%d,", &totalMsgNum, &curMsgNum);

        /* HI_VOID gsv num 4 str which have random wrong string */
        if (curMsgNum <= HI_GPSMNG_GSV_MAX_MSG_NUM)
        {
            g_gpsRawdataBuff.gpsGPGSVMsgTotalNum = (totalMsgNum > HI_GPSMNG_GSV_MAX_MSG_NUM) ? HI_GPSMNG_GSV_MAX_MSG_NUM : totalMsgNum;
            g_gpsRawdataBuff.gpsGPGSVMsgCurNum = curMsgNum;
            snprintf(g_gpsRawdataBuff.gpsRawData.gsvStr[curMsgNum - 1], HI_GPSMNG_MESSAGE_MAX_LEN, "%s", msgStr);

            if (g_gpsRawdataBuff.gpsGPGSVMsgTotalNum == g_gpsRawdataBuff.gpsGPGSVMsgCurNum)
            {
                g_gpsRawdataBuff.msgCount |= 1 << 5;
            }
        }
    }

    return HI_SUCCESS;
}

static HI_S32 GPS_DataProc(HI_VOID)
{
    HI_S32 i;
    HI_S32 ret = HI_SUCCESS;
	printf("GPS_DataProc(HI_VOID)-------------------\n");
    ret = GPSMNG_ParseRawData(&g_gpsRawdataBuff.gpsRawData, &g_gpsMngPacket);

    if (ret != HI_SUCCESS)
    {
        MLOGE("Parser GPS Raw data failed!\n");
    }

    HI_MUTEX_LOCK(g_gpsMngCtx.gpsLock);

    for (i = 0; i < HI_GPSMNG_CALLBACK_MAX_NUM; i++)
    {
        if (NULL != g_gpsMngCtx.fnGpsCB[i].fnGpsDataCB)
        {
            g_gpsMngCtx.fnGpsCB[i].fnGpsDataCB(&g_gpsMngPacket, g_gpsMngCtx.fnGpsCB[i].privateData);
        }
    }

    memset(&g_gpsRawdataBuff, 0x0, sizeof(GPSMNG_RAWDATA_BUFF));
    HI_MUTEX_UNLOCK(g_gpsMngCtx.gpsLock);

    return HI_SUCCESS;
}

HI_VOID* MNG_GPS_MsgReadThread(HI_VOID* para)
{
    /* if raw date end have "\n" and "\r", GPSMNG_MsgProc will  proc twice: reset variable prevent thise case,just proc once */
    HI_BOOL reset = HI_FALSE;
    HI_S32 ret = HI_SUCCESS;
    HI_U8 recvNum = 0;
    HI_U8 recv = 0;
    HI_CHAR buffer[HI_GPSMNG_MESSAGE_MAX_LEN];
    HI_GPSDATA gpsData = {0};
    gpsData.wantReadLen= 1;
    prctl(PR_SET_NAME, (HI_UL)"GPS_MsgReadThread", 0, 0, 0);

    while (g_gpsMngCtx.gpsReadFlg) 
	{
		
        ret = HI_HAL_GPS_GetRawData(&gpsData, GPSMNG_GET_DATA_TIMEOUT_MS);
		
        if(ret == HI_HAL_EAGAIN) 
		{
            continue;

        }
		
		else if (ret == HI_HAL_TIMEOUT) 
		{
			
            if ((g_gpsRawdataBuff.msgCount & GPSMNG_RAW_DATA_MASK) == GPSMNG_RAW_DATA_MASK) 
			{
                GPS_DataProc();
            }
        } 
		else if (ret == HI_HAL_EINVOKESYS) 
		{
			
            MLOGE("error\n");
            HI_usleep(1000 * 1000);
        } 
		else 
		{
            if (gpsData.actualReadLen == 0) 
			{
               	continue;
           	}

            recv = gpsData.rawData[0];

            if (reset && (recv != '\n' && recv != '\r')) 
			{

                if (recvNum >= (HI_GPSMNG_MESSAGE_MAX_LEN - 1)) 
				{
                    MLOGE("should expand buff size,discard data[%c]\n", recv);
                    buffer[HI_GPSMNG_MESSAGE_MAX_LEN - 1] = '\n';
                    continue;
                }

                buffer[recvNum] = recv;
                recvNum++;
            }

            if (recv == '$') 
			{
                reset = HI_TRUE;
                recvNum = 0;
                memset(buffer, 0 , HI_GPSMNG_MESSAGE_MAX_LEN);
                /* not to cut '$' symbol*/
                buffer[recvNum] = recv;
                recvNum++;
            }

            if ((recv == '\n' || recv == '\r') && reset) 
			{
                reset = HI_FALSE;
                GPSMNG_MsgProc(buffer);
            }

        }

    }

    return NULL;
}

HI_S32 HI_GPSMNG_Init(HI_VOID)
{
    HI_S32 ret = HI_SUCCESS;

    if (g_gpsMngCtx.gpsMngState != GPSMNG_STATE_DEINITED)
    {
        MLOGE("gps mng has already init or start!\n");
        return HI_GPSMNG_ENOTINIT;
    }

    HI_MUTEX_INIT_LOCK(g_gpsMngCtx.gpsLock);

    ret = HI_HAL_GPS_Init();

    if (ret != HI_SUCCESS)
    {
        MLOGE("hal gps init failed!\n");
        return HI_GPSMNG_EINTER;
    }

    g_gpsMngCtx.gpsMngState = GPSMNG_STATE_INITED;

    return ret;
}

HI_S32 HI_GPSMNG_Start(HI_VOID)
{
    HI_S32 ret = HI_SUCCESS;

    if (g_gpsMngCtx.gpsMngState == GPSMNG_STATE_DEINITED)
    {
        MLOGE("gps mng has not init!\n");
        return HI_GPSMNG_ENOTINIT;
    }
    else if (g_gpsMngCtx.gpsMngState == GPSMNG_STATE_STARTED)
    {
        MLOGE("gps mng has already start!\n");
        return HI_SUCCESS;
    }

    if (HI_SUCCESS != ret)
    {
        MLOGE("gps mng has already start!\n");
        return HI_GPSMNG_EALREADYSTART;
    }
	
    g_gpsMngCtx.gpsReadFlg = HI_TRUE;
    ret = pthread_create(&g_gpsMngCtx.gpsReadThd, 0, MNG_GPS_MsgReadThread, HI_NULL);

    if (ret != HI_SUCCESS)
    {
        MLOGE("create MsgReadThread failed\n");
        return HI_GPSMNG_EREGISTEREVENT;
    }

    MLOGI("HI_GPSMNG_Start END !\n");
    g_gpsMngCtx.gpsMngState = GPSMNG_STATE_STARTED;
    return ret;
}

HI_S32 HI_GPSMNG_Register(HI_GPSMNG_CALLBACK* pfnGpsCB)
{
    HI_S32 recordIdx = -1;
    HI_S32 i;

    g_gpsMngCtx.gpsRefreshData = HI_TRUE;

    for (i = 0; i < HI_GPSMNG_CALLBACK_MAX_NUM; i++)
    {
        if (NULL == g_gpsMngCtx.fnGpsCB[i].fnGpsDataCB)
        {
            recordIdx = i;
        }

        if (pfnGpsCB->fnGpsDataCB == g_gpsMngCtx.fnGpsCB[i].fnGpsDataCB \
            && pfnGpsCB->privateData == g_gpsMngCtx.fnGpsCB[i].privateData)
        {
            return HI_SUCCESS;
        }
    }

    if (-1 != recordIdx)
    {
        HI_MUTEX_LOCK(g_gpsMngCtx.gpsLock);
        g_gpsMngCtx.fnGpsCB[recordIdx].fnGpsDataCB = pfnGpsCB->fnGpsDataCB;
        g_gpsMngCtx.fnGpsCB[recordIdx].privateData = pfnGpsCB->privateData;
        HI_MUTEX_UNLOCK(g_gpsMngCtx.gpsLock);
        return HI_SUCCESS;
    }

    return HI_GPSMNG_EREGISTER;
}

HI_S32 HI_GPSMNG_UnRegister(HI_GPSMNG_CALLBACK* pfnGpsCB)
{
    HI_S32 i;

    g_gpsMngCtx.gpsRefreshData = HI_FALSE;

    for (i = 0; i < HI_GPSMNG_CALLBACK_MAX_NUM; i++)
    {
        if ((pfnGpsCB->privateData == g_gpsMngCtx.fnGpsCB[i].privateData) && \
            (pfnGpsCB->fnGpsDataCB == g_gpsMngCtx.fnGpsCB[i].fnGpsDataCB))
        {
            HI_MUTEX_LOCK(g_gpsMngCtx.gpsLock);
            g_gpsMngCtx.fnGpsCB[i].fnGpsDataCB = NULL;
            g_gpsMngCtx.fnGpsCB[i].privateData = NULL;
            HI_MUTEX_UNLOCK(g_gpsMngCtx.gpsLock);
            return HI_SUCCESS;
        }
    }

    return HI_GPSMNG_EUREGISTER;
}

HI_S32 HI_GPSMNG_Stop(HI_VOID)
{
    HI_S32 ret = HI_SUCCESS;

    if (g_gpsMngCtx.gpsMngState == GPSMNG_STATE_DEINITED)
    {
        MLOGE("gps mng has not init!\n");
        return HI_GPSMNG_ENOTINIT;
    }

    if (GPSMNG_STATE_STOP == g_gpsMngCtx.gpsMngState)
    {
        MLOGE("gps mng has already stop!\n");
        return HI_SUCCESS;
    }

    g_gpsMngCtx.gpsReadFlg = HI_FALSE;
    ret = pthread_join(g_gpsMngCtx.gpsReadThd, HI_NULL);

    if (ret != HI_SUCCESS)
    {
        MLOGE("join MsgReadThread failed\n");
        return HI_GPSMNG_EREGISTEREVENT;
    }

    g_gpsMngCtx.gpsMngState = GPSMNG_STATE_STOP;
    return ret;
}

HI_S32 HI_GPSMNG_Deinit(HI_VOID)
{
    HI_S32 ret = HI_SUCCESS;
    if (GPSMNG_STATE_STARTED == g_gpsMngCtx.gpsMngState)
    {
        MLOGE("gps must stoped!\n");
        return HI_FAILURE;
    }

    if (GPSMNG_STATE_DEINITED == g_gpsMngCtx.gpsMngState)
    {
        MLOGE("gps mng has already deinit!\n");
        return HI_SUCCESS;
    }

    ret = HI_HAL_GPS_Deinit();

    if (ret != HI_SUCCESS)
    {
        MLOGE("hal gps init failed!\n");
        return HI_GPSMNG_EDEINIT;
    }

    g_gpsMngCtx.gpsMngState = GPSMNG_STATE_DEINITED;

    HI_MUTEX_DESTROY(g_gpsMngCtx.gpsLock);
    return ret;
}

HI_S32 HI_GPSMNG_GetData(HI_GPSMNG_MSG_PACKET* msgPacket)
{
    HI_S32 ret = HI_SUCCESS;

    if (g_gpsMngCtx.gpsMngState != GPSMNG_STATE_STARTED)
    {
        MLOGE("gps mng has not start!\n");
        return HI_GPSMNG_EGETDATA;
    }
    memcpy(msgPacket, &g_gpsMngPacket, sizeof(HI_GPSMNG_MSG_PACKET));

    return ret;
}
/*
HI_GPSMNG_MSG_PACKET GPSpacket_return()
{
	
	HI_GPSMNG_Start();
	
	return g_gpsMngPacket;
}
*/
/* vim: set ts=4 sw=4 et: */
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
