#include "MSRLib.h"
HANDLE		idComDev;
DCB         lpDCB; 
COMMTIMEOUTS TIMETEST;
COMSTAT Comstate;
DWORD dwErrorMask;
DWORD DWOutData;
DWORD DWInData;
MSRHandler *msgHandler;
int numMsgHandlerEntries;
void *extraPtr;
bool lastReadWasRaw;
int MSRLib_Connect(char *portname, unsigned long baud) {
	    idComDev = CreateFile(portname, GENERIC_READ|GENERIC_WRITE,
		        	0, NULL, OPEN_EXISTING, 0, NULL);
    if (idComDev==INVALID_HANDLE_VALUE)
		  return -1;
	if (GetCommState(idComDev,&lpDCB)==FALSE)
		  return -1;
	lpDCB.BaudRate =baud;
	lpDCB.ByteSize  =8;
    lpDCB.Parity = NOPARITY;
	lpDCB.StopBits = ONESTOPBIT;
	if (SetCommState(idComDev,&lpDCB)==FALSE)
          return -1;
    if (SetupComm(idComDev,1024,1024)==FALSE)
          return -1;

    if (GetCommTimeouts(idComDev,&TIMETEST)==FALSE)
         return -1;
	TIMETEST.ReadIntervalTimeout = 100;
	TIMETEST.ReadTotalTimeoutMultiplier = 1;
    TIMETEST.ReadTotalTimeoutConstant = 1000;
    TIMETEST.WriteTotalTimeoutConstant =100;
	TIMETEST.WriteTotalTimeoutMultiplier =100;
    if (SetCommTimeouts(idComDev,&TIMETEST)==FALSE)
	    return -1;
    else
		return 0;
}
void MSRLib_Disconnect() {
	CloseHandle(idComDev);
}
void MSRLib_RegisterHandler(MSRHandler *handler, int numEntries, void *extra) {
	extraPtr = extra;
	msgHandler = handler;
	numMsgHandlerEntries = numEntries;
}
bool MSRLib_SetCoercivity(bool loco) {
	EMSRMessageCode code = EMSRMsg_SetLoCo;
	if(!loco) {
		code = EMSRMsg_SetHiCo;
	}
	return MSRLib_SendMessage(code,NULL,0);
}
bool MSRLib_GetCoercivity() {
	return MSRLib_SendMessage(EMSRMsg_GetCoercivity,NULL,0);
}
bool MSRLib_ReadISO() {
	lastReadWasRaw = false;
	return MSRLib_SendMessage(EMSRMsg_ReadISO,NULL,0);
}
bool MSRLib_ReadRaw() {
	lastReadWasRaw = true;
	return MSRLib_SendMessage(EMSRMsg_ReadRaw,NULL,0);
}
bool MSRLib_CommTest() {
	return MSRLib_SendMessage(EMSRMsg_CommTest,NULL,0);
}
bool MSRLib_RamTest() {
	return MSRLib_SendMessage(EMSRMsg_RamTest,NULL,0);
}
bool MSRLib_Reset() {
	return MSRLib_SendMessage(EMSRMsg_Reset,NULL,0);
}
bool MSRLib_SensorTest() {
	return MSRLib_SendMessage(EMSRMsg_SensorTest,NULL,0);
}
bool MSRLib_EraseCard(bool track1, bool track2, bool track3) {
	char selectbyte = 0;
	if(track1)
		selectbyte |= (1<<0);
	if(track2)
		selectbyte |= (1<<1);
	if(track3)
		selectbyte |= (1<<2);
	return MSRLib_SendMessage(EMSRMsg_EraseCard, (char *)&selectbyte, sizeof(char));
}
bool MSRLib_GetDeviceVersion() {
	return MSRLib_SendMessage(EMSRMsg_GetDeviceVersion,NULL,0);
}
bool MSRLib_GetModelVersion() {
	return MSRLib_SendMessage(EMSRMsg_GetModelVersion,NULL,0);
}
bool MSRLib_CheckLeadingZeros() {
	return MSRLib_SendMessage(EMSRMsg_CheckLeadingZeros,NULL,0);
}
bool MSRLib_SetBPC(char b1, char b2, char b3) { //untested
	char senddata[3];
	senddata[0] = b1;
	senddata[1] = b2;
	senddata[2] = b3;
	return MSRLib_SendMessage(ESRMsg_SetBPC, (char *)&senddata, sizeof(senddata));
}
bool MSRLib_SetLED(bool green, bool yellow, bool red) {
	bool err = false;
	MSRLib_SetAllLED(false);
	if(green && !MSRLib_SendMessage(EMSRMsg_SetGreenOn,NULL,0))
		err = true;
	if(yellow && !MSRLib_SendMessage(EMSRMsg_SetYellowOn,NULL,0))
		err = true;
	if(red &&!MSRLib_SendMessage(EMSRMsg_SetRedOn,NULL,0))
		err = true;
	return !err;
}
bool MSRLib_SetAllLED(bool on) {
	EMSRMessageCode code = EMSRMsg_AllLEDOn;
	if(!on) {
		code = EMSRMsg_AllLEDOff;
	}
	return MSRLib_SendMessage(code,NULL,0);
}
bool MSRLib_SendMessage(EMSRMessageCode code, char *msg, int len, bool shouldRead) {
	bool err = false;
	char *buff = (char *)malloc(len + 2);
	if(buff == NULL) return false;
	//add header
	buff[0] = MSR_MsgChar;
	buff[1] = code;
	if(msg != NULL)
		memcpy(&buff[2],msg, len);
	//PurgeComm(idComDev,PURGE_TXCLEAR);
	//PurgeComm(idComDev,PURGE_RXCLEAR);
	len += 2; //include msg header len
	if (WriteFile(idComDev,buff,len,&DWOutData,NULL)==FALSE) err = true;
	if (DWOutData < DWORD(len)) err = true;
	if(shouldRead) {
		if(!MSRLib_WaitMessage(1000)) err = true;
	}
	free(buff);
    return !err;
}
bool MSRLib_WaitMessage(unsigned int timeout) {
	COMMTIMEOUTS ctimeout;
    unsigned char buff[512];
	int len = sizeof(buff);
	if(GetCommTimeouts(idComDev,&ctimeout) == FALSE) {
		return false;
	}
	ctimeout.ReadTotalTimeoutConstant = timeout;
	if(SetCommTimeouts(idComDev,&ctimeout) == FALSE) {
		return false;
	}
    if (ReadFile(idComDev,&buff,len,&DWInData,NULL)==FALSE) return false;
	if (DWInData < 1) return false;
	MSRHandler *handler = findHandlerByCode((EMSRMessageCode)buff[1]);
	if(handler != NULL) {
		handler->HandlerFunc((EMSRMessageCode)buff[1],(char *)&buff[2],DWInData-2,extraPtr);
	}
	return true;
}
void MSRLib_BuildTrackData(TrackData *data, char *out, int &outlen, bool useISO) {
}
bool MSRLib_Write(TrackData *data, bool useISO) {
	char buff[512];
	int idx = 0;
	buff[idx++] = MSR_MsgChar;
	buff[idx++] = 's';
	buff[idx++] = MSR_MsgChar;
	buff[idx++] = 1; //track number
	if(!useISO)
		buff[idx++] = data->track1len;
	for(int i=0;i<data->track1len;i++) {
		buff[idx++] = data->track1Data[i];
	}
	buff[idx++] = MSR_MsgChar;
	buff[idx++] = 2; //track number
	if(!useISO)
		buff[idx++] = data->track2len;
	for(int i=0;i<data->track2len;i++) {
		buff[idx++] = data->track2Data[i];
	}
	buff[idx++] = MSR_MsgChar;
	buff[idx++] = 3; //track number
	if(!useISO)
		buff[idx++] = data->track3len;
	for(int i=0;i<data->track3len;i++) {
		buff[idx++] = data->track3Data[i];
	}

	//terminator
	buff[idx++] = '?';
	buff[idx++] = 0x1c;
	return MSRLib_SendMessage(useISO?EMSRMsg_WriteISO:EMSRMsg_WriteRaw,(char *)&buff,idx);
}
MSRHandler *findHandlerByCode(EMSRMessageCode code) {
	for(int i=0;i<numMsgHandlerEntries;i++) {
		if(msgHandler[i].HandlerCode == code) {
			return &msgHandler[i];
		}
	}
	return NULL;
}
//trackDataMode being -1 means assume the data being read is the last read mode(raw/iso)
//tdm being 1 = raw, else iso
void MSRLib_ReadTrackData(TrackData **out, char *data, int len, int trackDataMode) {
	TrackData *trackData = (TrackData *)malloc(sizeof(TrackData)+1);
	memset(trackData,0,sizeof(TrackData));
	char *p;
	char *curtrack;
	int buffidx;
	bool trackMode = lastReadWasRaw;
	if(trackDataMode != -1) {
		if(trackDataMode == 1) {
			trackMode = true;
		} else trackMode = false;
	}
	int trackNum = 0,trackLen;
	for(int i=0;i<len;i++) {
		if(data[i] == MSR_MsgChar) {
			if(data[i+1] >= 1 && data[i+1] <= 3) {
				trackNum = data[i+1];
				if(!trackMode) {
					p = strchr(&data[i+1], MSR_MsgChar);
					if(p != NULL) {
						p = (char *)(p-data);
					}
					trackLen = (int)p;
				} else {
					trackLen = data[i+2];
					i++; //skip track len(note this isn't in order, but since theres no reading from data now it doesn't matter)
				}
				curtrack = (char *)malloc(trackLen);
				switch(trackNum) {
				case 1: {
					trackData->track1Data = curtrack;
					trackData->track1len = trackLen;
					break;
					}
				case 2: {
					trackData->track2Data = curtrack;
					trackData->track2len = trackLen;
					break;
					}
				case 3: {
					trackData->track3Data = curtrack;
					trackData->track3len = trackLen;
					break;
					}
				}
				memset(curtrack,0,trackLen+1);
			}
			i++; //skip track num
			buffidx = 0;
			continue;
		} else if(data[i] == 0x1C && data[i+1] == 0x1B) { //status bit is next
			trackData->status = data[i+2];
			break;
		}
		curtrack[buffidx++] = data[i];
	}
	*out = trackData;
}
void MSRLib_Think() {
	MSRLib_WaitMessage(1);
}