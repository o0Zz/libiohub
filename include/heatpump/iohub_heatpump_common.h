#pragma once

typedef enum
{
	HeatpumpMode_None = 0,
	
	HeatpumpMode_Cold,
	HeatpumpMode_Dry,
	HeatpumpMode_Fan,
	HeatpumpMode_Auto,
	HeatpumpMode_Heat,
	
	HeatpumpMode_Count
}IoHubHeatpumpMode;

typedef enum
{
	HeatpumpFanSpeed_None = 0,
	HeatpumpFanSpeed_Auto,
	HeatpumpFanSpeed_High,
	HeatpumpFanSpeed_Med,
	HeatpumpFanSpeed_Low,
	
	HeatpumpFanSpeed_Count
}IoHubHeatpumpFanSpeed;

typedef enum
{
	HeatpumpAction_OFF = 0,
	HeatpumpAction_ON,
	HeatpumpAction_Direction,
}IoHubHeatpumpAction;

typedef enum
{
	HeatpumpVaneMode_Auto = 0,
	HeatpumpVaneMode_1,
	HeatpumpVaneMode_2,
	HeatpumpVaneMode_3,
	HeatpumpVaneMode_4,
	HeatpumpVaneMode_5,
	HeatpumpVaneMode_Swing,

	HeatpumpVaneMode_Count
}IoHubHeatpumpVaneMode;

typedef struct IoHubHeatpumpSettings_s
{
	IoHubHeatpumpAction		mAction;
	int						mTemperature;
	IoHubHeatpumpFanSpeed	mFanSpeed;
	IoHubHeatpumpMode		mMode;
	IoHubHeatpumpVaneMode	mVaneMode;
}IoHubHeatpumpSettings;
