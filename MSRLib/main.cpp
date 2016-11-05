#include <windows.h>
#include <stdio.h>
#include "MSRLib.h"
bool CommTestHandler(EMSRMessageCode code,char *data, int len, void *extra);
bool CoercivityModeHandler(EMSRMessageCode code,char *data, int len, void *extra);
bool ReadHandler(EMSRMessageCode code,char *data, int len, void *extra);
bool ErrorOKHandler(EMSRMessageCode code,char *data, int len, void *extra);
bool ErrorFailHandler(EMSRMessageCode code,char *data, int len, void *extra);
MSRHandler handlerTable[] = {
	{EMSRMsg_CommOK,CommTestHandler},
	{EMSRMsg_HiCoMode,CoercivityModeHandler},
	{EMSRMsg_LoCoMode,CoercivityModeHandler},
	{EMSRMsg_Read,ReadHandler},
	{EMSRMsg_TestOK, ErrorOKHandler},
	{EMSRMsg_TestFail, ErrorFailHandler},
	{EMSRMsg_IOError, ErrorFailHandler},
	{EMSRMsg_CommandFormatError, ErrorFailHandler},
	{EMSRMsg_InvalidCommand, ErrorFailHandler},
	{EMSRMsg_InvalidWriteMode, ErrorFailHandler},
};
int main() {
	MSRLib_Connect("COM4", 9000);
	bool oktoWriteAgain = true;
	MSRLib_RegisterHandler((MSRHandler *)&handlerTable,sizeof(handlerTable)/sizeof(MSRHandler), &oktoWriteAgain);
	MSRLib_CommTest();
	MSRLib_Reset();
	MSRLib_CheckLeadingZeros();
	TrackData data;
	memset(&data,0,sizeof(TrackData));
	data.track2Data = (char *)malloc(64);
	int cardnumber = 1;

	while(true) {
		if(oktoWriteAgain) {
			printf("next number to write: %d\n",cardnumber);
			sprintf(data.track2Data,"%d",cardnumber++);
			data.track2len = strlen(data.track2Data);
			MSRLib_Write(&data,true);
			MSRLib_SetLED(true,false,true);
			oktoWriteAgain = false;
		}
		MSRLib_Think();
	
	}
	MSRLib_Disconnect();
	return 0;
}
bool CoercivityModeHandler(EMSRMessageCode code,char *data, int len, void *extra) {
	if(code == EMSRMsg_HiCoMode) {
		printf("hico mode\n");
	} else if(code == EMSRMsg_LoCoMode) {
		printf("loco mode\n");
	}
	return true;
}
bool ReadHandler(EMSRMessageCode code,char *data, int len, void *extra) {
	TrackData *trackData;
	MSRLib_ReadTrackData(&trackData, data, len);
	printf("%s %s %s %c!\n",trackData->track1Data,trackData->track2Data,trackData->track3Data,trackData->status);
	return true;
}
bool CommTestHandler(EMSRMessageCode code,char *data, int len, void *extra) {
	printf("comm OK\n");
	return false;
}
bool ErrorOKHandler(EMSRMessageCode code,char *data, int len, void *extra) {
	bool *oktowrite = (bool *)extra;
	*oktowrite = true;
	printf("ok handler\n");
	return false;
}
bool ErrorFailHandler(EMSRMessageCode code,char *data, int len, void *extra) {
	printf("read/write error\n");
	MSRLib_Reset();
	bool *oktowrite = (bool *)extra;
	*oktowrite = true;
	return false;
}