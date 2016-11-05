#include <windows.h>
#include <stdio.h>
#define	TrackErr_Blank	    0x71
#define TrackErr_Head	    0x72
#define TrackErr_Tail	    0x73
#define TrackErr_Parity	    0x74
#define TrackErr_LRC	    0x75
#define TrackErr_Timeout    0x76
#define MSR_MsgChar 27 //ESC
enum EMSRMessageCode {
	//OUT to DEVICE
	EMSRMsg_CommTest = 0x65,
	EMSRMsg_AllLEDOff = 0x81,
	EMSRMsg_AllLEDOn = 0x82,
	EMSRMsg_Reset = 0x61,
	EMSRMsg_ReadRaw = 0x6D,
	EMSRMsg_WriteRaw = 0x6E,
	EMSRMsg_SensorTest = 0x86,
	EMSRMsg_ReadISO = 0x72,
	EMSRMsg_WriteISO = 0x77,
	EMSRMsg_RamTest = 0x87,
	EMSRMsg_GetDeviceVersion = 0x74,
	EMSRMsg_GetModelVersion = 0x76,
	EMSRMsg_SetHiCo = 0x78,
	EMSRMsg_SetLoCo = 0x79,
	EMSRMsg_GetCoercivity = 0x64,
	EMSRMsg_SetGreenOn = 0x83,
	EMSRMsg_SetYellowOn = 0x84,
	EMSRMsg_SetRedOn = 0x85,
	EMSRMsg_EraseCard = 0x63,
	EMSRMsg_CheckLeadingZeros = 0x6C,
	ESRMsg_SetBPC = 0x6F,
	//IN to PC
	EMSRMsg_CommOK = 0x79,
	EMSRMsg_Read = 0x73,
	EMSRMsg_TestOK = 0x30, //whatever test just happend, it was a success
	EMSRMsg_IOError = 0x31,
	EMSRMsg_CommandFormatError = 0x32,
	EMSRMsg_InvalidCommand = 0x34,
	EMSRMsg_InvalidWriteMode = 0x39,
	EMSRMsg_TestFail = 0x41, //test failed
	EMSRMsg_SetBPCResp = 0x30,

	EMSRMsg_HiCoMode = 0x68,
	EMSRMsg_LoCoMode = 0x6C,

};
typedef struct {
	EMSRMessageCode HandlerCode;
	bool (*HandlerFunc)(EMSRMessageCode ,char *, int, void *);
} MSRHandler;
typedef struct {
	char *track1Data;
	int track1len;
	char *track2Data;
	int track2len;
	char *track3Data;
	int track3len;
	char status;
} TrackData;
void MSRLib_RegisterHandler(MSRHandler *handler, int numEntries, void *extra);
int MSRLib_Connect(char *portname, unsigned long baud);
void MSRLib_Disconnect();

bool MSRLib_CommTest();
bool MSRLib_SensorTest();
bool MSRLib_Reset();
bool MSRLib_ReadISO(); //read in ISO format
bool MSRLib_ReadRaw();
bool MSRLib_Write(TrackData *data, bool useISO);
bool MSRLib_CheckLeadingZeros();
bool MSRLib_RamTest();
bool MSRLib_GetDeviceVersion();
bool MSRLib_GetModelVersion();
bool MSRLib_SetCoercivity(bool loco); //if false its hico
bool MSRLib_GetCoercivity();
bool MSRLib_SetLED(bool green, bool yellow, bool red);
bool MSRLib_SetAllLED(bool on);
bool MSRLib_SendMessage(EMSRMessageCode code, char *msg, int len, bool shouldRead = false);
bool MSRLib_WaitMessage(unsigned int timeout);
bool MSRLib_EraseCard(bool track1, bool track2, bool track3);
bool MSRLib_SetBPC(char b1, char b2, char b3); //set bits per character
void MSRLib_Think(); //checks if a new message is recieved and calls the required handler

MSRHandler *findHandlerByCode(EMSRMessageCode code);
void MSRLib_ReadTrackData(TrackData **out, char *data, int len, int trackDataMode = -1);