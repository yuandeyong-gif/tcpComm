#ifndef GPS_ADPATER_H_
#define GPS_ADPATER_H_
#include "apt32f172.h"
#include "iostring.h"

#define MLOGD my_printf
#define MLOGI my_printf
#define MLOGW my_printf
#define MLOGE my_printf

typedef char HI_CHAR;

typedef uint32_t HI_U32;
typedef int HI_S32;
typedef char HI_BOOL;

typedef uint8_t HI_U8;
typedef void HI_VOID;


#define HI_FALSE  0
#define HI_TRUE   1
#define HI_NULL   NULL
#define HI_SUCCESS 0
define  HI_FAILURE  (-1)

#endif

