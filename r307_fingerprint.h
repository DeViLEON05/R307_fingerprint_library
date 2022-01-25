#ifndef R307_FINGERPRINT_H
#define R307_FINGERPRINT_H
// This library got some of its approach/ ideas from Adafruit Fingerprint Library
//** Created by Patrick James O. De Leon **//
#include "Arduino.h"
#if defined(__AVR__) || defined(ESP8266)
	#define mcuNeedSoftwareSerial true
	#include <SoftwareSerial.h>
#elif defined(FREEDOM_E300_HIFIVE1)
	#include <SoftwareSerial32.h>
	#define SoftwareSerial SoftwareSerial32
	#define mcuNeedSoftwareSerial true
#endif

#ifndef mcuNeedSoftwareSerial
	#define mcuNeedSoftwareSerial false
#endif

/* START - Source: https://www.rhydolabz.com/documents/37/r307-fingerprint-module-user-manual.pdf */
// PS Some of defined naming ideas came from adafruit (Some I just copied because it was the best name)
// == Instruction Code Function Definition //
	// Other variables
	#define FP_HEADER 0xEF01       // startup command of the Serial Protocol of the R307 FP
	#define FP_ADDRESS 0xFFFFFFFF  // change this if you changed the default fp module address
	#define FP_PASSWORD 0x00000000 // change this if you changed the default fp password
	#define FP_BAUDRATE 115200	   // baudrate used to communicate with the Fingerprint
	#define FP_TIMEOUT 2000		   // FP UART Communication Timeout
	#define FP_SERIALDEBUG true		   // Serial debugging of the FP - set it to true to enable serial debugging 
	
	#define FP_CMDPACKET 0x1 // Command packet
	#define FP_DATAPACKET 0x2 // Data packet, must follow command packet or acknowledge packet
	#define FP_ACKNOWLEDGEPACKET 0x7 // Acknowledge packet
	#define FP_ENDPACKET 0x8 // End of data packet
	
	// System Related Commands
	#define FP_PASSWORDVERIFY 0x13 // to verify password
	#define FP_PASSWORDSET 0x12 // to set password
	#define FP_DEVADDSET 0x15 // to set device address
	#define FP_SYSTEMPARAMSET 0x0E // to set system parameter
	#define FP_PORTCONTROL 0x17 // port control
	#define FP_SYSTEMPARAMREAD 0x0F // to read system parameter
	#define FP_TEMPLATECOUNT 0x1D // to read finger template numbers
	// Fingerprint Processing Commands
	#define FP_IMAGEGENERATE 0x01 // collect finger image and store it in image buffer
	#define FP_IMAGEDOWNLOAD 0x0A // upload image from image buffer to upper computer
	#define FP_IMAGEUPLOAD 0x0B // download image from upper computer to image buffer
	#define FP_IMAGETOCHAR 0x02 // generate character file from image
	#define FP_TEMPLATEGENERATE 0x05 //  combine character files from character buffer 1 and 2 and generate template
	#define FP_TEMPLATEDOWNLOAD 0x08 // upload template
	#define FP_TEMPLATEUPLOAD 0x09 // download template
	#define FP_TEMPLATESTORE 0x06 // store template
	#define FP_TEMPLATELOAD 0x07 // load / read template
	#define FP_TEMPLATEDELETE 0x0C // delete templates
	#define FP_LIBRARYCLEAR 0x0D // empty library or removes all templates
	#define FP_TEMPLATEMATCHING 0x03 // precise matching of two templates
	#define FP_FINGERSEARCH 0x04 // search the finger library 
	#define FP_FASTFINGERSEARCH 0x1B // search the library fastly only using character buffer 1
	// Other Commands
	#define FP_GETRANDOMCODE 0x14 // get random code - don't know the purpose of this
	#define FP_NOTEPADWRITE 0x18 // write note pad
	#define FP_NOTEPADREAD 0x19 // read note pad
	
// == Confirmation Code Definition //
	#define FP_OK 0x00 // command execution complete
	#define FP_RECEIVEPACKAGEFAIL 0x01 // error when receiving data package
	#define FP_NOFINGER_A 0x02 // no finger on the sensor
	#define FP_ENROLLFINGERFAIL 0x03 // fail to enroll finger
	// START Same Error multiple meaning - fail to generate character file due to.... //
	#define FP_GENERATECHARFAIL_A 0x04 // over-disorderly fingerprint image
	#define FP_GENERATECHARFAIL_B 0x05 // over-wet fingerprint image
	#define FP_GENERATECHARFAIL_C 0x06 // over-disorderly fingerprint image == duplicate error?
	#define FP_GENERATECHARFAIL_D 0x07 // lackness of char point or over-smallness of fingerprint image
	// END
	#define FP_FINGERSMISMATCH 0x08 // finger doesn't match
	#define FP_FINGERMATCHFAIL 0x09 // failed to find matching finger
	#define FP_CHARCOMBINEFAIL 0x0A // failed to combine the character files
	#define FP_BADLOCATION 0x0B //addressing PAGEID is beyond the finger library
	#define FP_TEMPLATEREADFAIL 0x0C // error when reading template from library or template is invalid
	#define FP_TEMPLATEUPLOADFAIL 0x0D // error when uploading template
	#define FP_MODULEDATARECEIVEFAIL 0x0E // module can't receive the following data packages
	#define FP_IMAGEUPLOADFAIL 0x0F // failed to upload image
	#define FP_TEMPLATEDELETEFAIL 0x10 // failed to delete the template
	#define FP_FINGERLIBRARYCLEARFAIL 0x11 // failed to clear fingerlibrary
	#define FP_PASSWORDFAIL 0x13 // fingerprint password wrong - default password is 0xFFFFFFFF
	#define FP_GENERATEIMAGEFAIL 0x15 // failed to generate the image for the lackness of valid primary image
	#define FP_FLASHWRITEFAIL 0x18 // error when writing flash
	#define FP_NODEFINITION 0x19 // no definition erro - not sure if it says error code is not defined or a parameter required is not defined causing this error
	#define FP_INVALIDREGISTERNO 0x1A // invalid register number
	#define FP_INVALIDREGISTERCONFIG 0x1B // invalid configuration of register
	#define FP_WRONGNOTEPADPAGE 0x1C // wrong notepad page number
	#define FP_COMMUNICATIONFAIL 0x1D // failed to operate the communication port or system is reserved
	#define FP_NOFINGER2 0x41 // no finger on sensor when adding fingerprint for the second time
	#define FP_ENROLLFINGERFAIL2 0x42 // failed to enroll the finger for second fingerprint add
	// START Same Error multiple meaning - fail to generate character file for second finger due to.... //
	#define FP_GENERATECHARFAIL2_A 0x43 // lackness of character point or over-smallness of fingerprint image
	#define FP_GENERATECHARFAIL2_B 0x44 // over-disorderly fingerprint image
	// END
	#define FP_ALREADYEXISTS 0x45 // finger already exists in library - duplicate fingerprint
	#define FP_BADRECEIVEDPACKET 0xFE // received packet is different or corrupted
	#define FP_RECEIVETIMEOUT 0xFF // timeout reached when receiving packet from fp
	#define FP_CODECRASH 0x90 // the library code was modified and reached lines that shouldn't be possible if not modified.
	#define FP_INVALIDVALUE 0x91 // the argument value is invalid
	#define FP_FUNCTIONREQUIREMENTNOTMET 0x92 // the function requirement are not met
	
struct R307_fp_packet {
	R307_fp_packet( uint8_t cmd_type, uint16_t dataLength, uint8_t *data, uint32_t address = FP_ADDRESS ) {
		this->cmd_type = cmd_type;
		this->cmd_length = dataLength;
		for( int a = 0; a < 4; a++ ) {
			this->cmd_address[a] = (uint8_t)(address >> (8*(3 - a)));
		}
		if( cmd_length < 256 ) 			// 256 bytes is the default data package length
			memcpy(this->cmd_data, data, cmd_length);
		else {
			memcpy(this->cmd_data, data, 256);
		}
	}
	uint16_t cmd_header = FP_HEADER; 	// Fingerprint Command Start
	uint8_t cmd_address[4];				// Device address - default is 0xFFFFFFFF
	uint8_t cmd_type;					// Command Function - see 'Instruction Code Function Definition'
										// 		- above to see defined command functions
	uint16_t cmd_length;				// length of command to send
	uint8_t cmd_data[256];				// raw buffer for payload
};

class R307_Fingerprint {
	public:
		//methods
		#if defined(__AVR__) || defined(ESP8266) || defined(FREEDOM_E300_HIFIVE1)
			R307_Fingerprint(SoftwareSerial *ss, uint32_t address = FP_ADDRESS, uint32_t password = FP_PASSWORD);
		#endif
		R307_Fingerprint(HardwareSerial *hs, uint32_t address = FP_ADDRESS, uint32_t password = FP_PASSWORD);
		R307_Fingerprint(Stream *serial, uint32_t address = FP_ADDRESS, uint32_t password = FP_PASSWORD);
		// initialize fp sensor communication
		void begin(uint32_t baudrate = FP_BAUDRATE);
		// functions for UI use
		boolean verifyPassword(uint32_t password = FP_PASSWORD);
		boolean setPassword(uint32_t newPassword = FP_PASSWORD);
		boolean setAddress(uint32_t newAddress = FP_ADDRESS);
		boolean setSystemParam(String mode, int value);
		boolean readSystemParam();
		int getTemplateCount();
		boolean generateFpImage();
		boolean downloadFpImage();
		boolean generateFpChar(int bufferId = 1);
		boolean generateFpTemplate();
		boolean downloadFpChar(int bufferId = 1);
		boolean storeFpTemplate(int pageId, int bufferId = 1);
		boolean loadFpTemplate(int pageId, int bufferId = 1);
		boolean deleteFpTemplate(int pageId, int numberOfTemplatesToDelete = 1);
		boolean emptyFpLibrary();
		boolean matchFpCharBuffers();
		boolean fpSearch(int bufferId = 1);
		/* TODO: START understand and make a functional code about these commands in the documentation of R307 Fp
		customFingerSearch(uint8_t captureTime, uint16_t startBit, uint16_t searchQuantity) - GR_Auto Search
		autoFingerVerify() - GR_Identify
		uploadFpImage() - DownImage
		uploadFpChar() - DownChar
		// TODO: END*/
		uint8_t sendData(int mode, uint8_t ic, uint32_t param1 = 0, uint8_t param2 = 0,
								   uint8_t param3 = 0, uint16_t param4 = 0, uint16_t param5 = 0);
		// functions to talk to fp sensor
		void sendPacket(const R307_fp_packet &packet);
		uint8_t receivePacket( R307_fp_packet *packet, uint16_t timeout = FP_TIMEOUT );
		//properties
		uint16_t status_reg;      // Status register (operation status of FP) - auto configured by readSystemParam function
		uint16_t system_id;       // System Id - auto configured by readSystemParam function
		uint16_t capacity;        // FP Capacity - auto configured by readSystemParam function
		uint16_t security_level;  // Security Level - auto configured by readSystemParam function
		uint32_t deviceAddress;   // Device Address - auto configured by readSystemParam function
		uint16_t packet_length;   // Packet Length - auto configured by readSystemParam function
		uint16_t baud_rate;       // FP Uart Baud Rate - auto configured by readSystemParam function
		int templateCount;   // FP valid template count - auto configured by getTemplateCount function
		//bool FP_SERIALDEBUG = false; - enable this after testing and remove the defined FP_SERIALDEBUG above
	private:
		// methods
		uint8_t receiveAdditionalPacket(uint16_t timeout = FP_TIMEOUT);
		String hexToString(uint8_t value);
		String errorCodeDictionary(uint8_t errorCode);
		
		
		//properties
		uint32_t devicePassword;
		uint8_t fp_content[256];
		int contentByteCounter;
		bool createdCharBuffer1 = false;
		bool createdCharBuffer2 = false;
		bool createdImageBuffer = false;
		bool systemParamRead = false;
		
		Stream *fpSerial;
		#if defined(__AVR__) || defined(ESP8266) || defined(FREEDOM_E300_HIFIVE1)
			SoftwareSerial *swSerial;
		#endif
		HardwareSerial *hwSerial;
};
#endif