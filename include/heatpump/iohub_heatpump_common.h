#ifndef _HEATPUMP_COMMON_H_
#define _HEATPUMP_COMMON_H_

typedef enum
{
	HeatpumpMode_None = 0,
	
	HeatpumpMode_Cold,
	HeatpumpMode_Dry,
	HeatpumpMode_Fan,
	HeatpumpMode_Auto,
	HeatpumpMode_Heat,
	
	HeatpumpMode_Count
}HeatpumpMode;

typedef enum
{
	HeatpumpFanSpeed_None = 0,
	HeatpumpFanSpeed_Auto,
	HeatpumpFanSpeed_High,
	HeatpumpFanSpeed_Med,
	HeatpumpFanSpeed_Low,
	
	HeatpumpFanSpeed_Count
}HeatpumpFanSpeed;

typedef enum
{
	HeatpumpAction_OFF = 0,
	HeatpumpAction_ON,
	HeatpumpAction_Direction,
}HeatpumpAction;

#endif