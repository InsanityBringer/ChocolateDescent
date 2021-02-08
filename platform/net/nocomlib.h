/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
//This include is just to allow compiling. It doesn't mean it will work. Values in here are only dummy values


//I just put in whatever felt good, consider this file nonesense.
typedef struct {
	int status;
	int count;

} PORT;

#define COM1	0
#define COM2	1
#define COM3	2
#define COM4	3

#define IRQ2	2
#define IRQ3	3
#define IRQ4	4
#define IRQ7	7
#define IRQ15	15

#define ASSUCCESS	1
#define ASBUFREMPTY	0	
#define TRIGGER_04	4

#define ON	1
#define OFF 0

//no idea why KRB didn't proto these
PORT* PortOpenGreenleafFast(int port, int baud, char parity, int databits, int stopbits);

void SetDtr(PORT* port, int state);

void SetRts(PORT* port, int state);

void UseRtsCts(PORT* port, int state);

void WriteChar(PORT* port, char ch);

void ClearRXBuffer(PORT* port);

void ReadBufferTimed(PORT* port, char* buf, int a, int b);

int Change8259Priority(int a);

int FastSetPortHardware(int port, int IRQ, int baseaddr);
void FastSet16550TriggerLevel(int a);
void FastSet16550UseTXFifos(int a);

void FastSavePortParameters(int port);
int PortClose(PORT* port);
void FastRestorePortParameters(int num);
int GetCd(PORT* port);
int ReadCharTimed(PORT* port, int blah);
int ReadChar(PORT* port);

void ClearLineStatus(PORT* port);
int HMInputLine(PORT* port, int a, char* buf, int b);
void HMWaitForOK(int a, int b);
int HMSendString(PORT* port, char* msg);
void HMReset(PORT* port);
void HMDial(PORT* port, char* pPhoneNum);
void HMSendStringNoWait(PORT* port, char* pbuf, int a);
void HMAnswer(PORT* port);
void ClearTXBuffer(PORT* port);
void WriteBuffer(PORT* port, char* pbuff, int len);

int GetLineStatus(PORT* port);
