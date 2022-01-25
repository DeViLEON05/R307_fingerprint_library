#include "r307_fingerprint.h"
//=====================================================================================
#define readPacket(...)											            \
	uint8_t packetData[] = {__VA_ARGS__};								    \
	R307_fp_packet packet(FP_CMDPACKET, sizeof(packetData), packetData);	\
	sendPacket(packet);											            \
	return receivePacket(&packet);
//=====================================================================================
#define writePacket(...)										            \
	readPacket(__VA_ARGS__);									            \
	return packet.cmd_data[0];
//=====================================================================================
//*******=======___Public Methods___=======*******//
#if mcuNeedSoftwareSerial
	R307_Fingerprint::R307_Fingerprint(SoftwareSerial *ss, uint32_t address, uint32_t password) {
		deviceAddress = address;
		devicePassword = password;
		
		hwSerial = NULL;
		swSerial = ss;
		fpSerial = swSerial;
	}
#endif
//=====================================================================================
R307_Fingerprint::R307_Fingerprint(HardwareSerial *hs, uint32_t address, uint32_t password) {
	deviceAddress = address;
	devicePassword = password;
	
	if( mcuNeedSoftwareSerial )
		swSerial = NULL;
	hwSerial = hs;
	fpSerial = hwSerial;
}
//=====================================================================================
R307_Fingerprint::R307_Fingerprint(Stream *serial, uint32_t address, uint32_t password) {
	deviceAddress = address;
	devicePassword = password;
	
	hwSerial = NULL;
	if( mcuNeedSoftwareSerial )
		swSerial = NULL;
	fpSerial = serial;
}
//=====================================================================================
/*
	@ description: Initializes the serial interface of mcu and fp
	@ arguments :
		baudrate -> baud rate allowed is the product of N * 9600,
					where N is a whole number ranging from 1 to 12
					default is 115200
	@ returns true if no problem encountered otherwise false
*/
void R307_Fingerprint::begin(uint32_t baudrate) {
	delay(500); // delay for fp boot up
	if(hwSerial) hwSerial->begin(baudrate);
	if(mcuNeedSoftwareSerial && swSerial) swSerial->begin(baudrate);
}
//=====================================================================================
/*
	@ description: Verifies fp's handshaking password
	@ arguments :
		password -> the 4 byte password of the fp
					default is 0x00000000
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::verifyPassword(uint32_t password) {
	uint8_t result = sendData(0, FP_PASSWORDVERIFY, password);
	String str = errorCodeDictionary(result); 
	if( str != "" && FP_SERIALDEBUG && Serial ) 
		Serial.println(str);
	else
		devicePassword = password;
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: Changes the handshaking password of the fp
	@ arguments :
		newPassword -> the new 4 byte password of the fp
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::setPassword(uint32_t newPassword) {
	if (!fpSerial) return FP_RECEIVEPACKAGEFAIL;
	
	uint8_t result = sendData(0, FP_PASSWORDSET, newPassword);
	String str = errorCodeDictionary(result); 
	if( str != "" && FP_SERIALDEBUG && Serial ) 
		Serial.println(str);
	else
		devicePassword = newPassword;
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: Changes the fp address
	@ arguments :
		newAddress -> the new 4 byte address of the fp
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::setAddress(uint32_t newAddress) {
	if (!fpSerial) return FP_RECEIVEPACKAGEFAIL;
	
	uint8_t result = sendData(0, FP_DEVADDSET, newAddress);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) 
		Serial.println(str);
	else
		deviceAddress = newAddress;
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: Changes the fp parameters baud rate, security level and packet length
	@ arguments :
		mode  -> String param that determines which param to modify
				 "baudRate" for baud rating, "securityLevel" for security level
				 and "packetLength" for packet length
		value -> the new value of the selected parameter that will be set.
				 value accepted for baudRate are 9600-115200 that are divisible by 9600
				 value accepted for securitLevel are 1-5
					where 1 is the lowest security and 5 is the highest
				 value accepted for packetLength are 32,64,128,256 and is using
					the formula 2^(5 + N) where N is ranged from 0-3 
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::setSystemParam(String mode, int value) {
	if (!fpSerial) return FP_RECEIVEPACKAGEFAIL;
	uint8_t paramNumber;
	uint8_t paramValue;
	String str = "Invalid argument values";
	if( mode == "baudRate" ) {
		if( value % 9600 != 0 || (value / 9600 < 1 || value / 9600 > 12) ) {
			if( FP_SERIALDEBUG && Serial ) Serial.println(str);
			return false;
		}
		paramNumber = 4;
		paramValue = value / 9600;
	} else if( mode == "securityLevel" ) {
		if( value < 1 || value > 5 ) {
			if( FP_SERIALDEBUG && Serial ) Serial.println(str);
			return false;
		}
		paramNumber = 5;
		paramValue = value;
	} else if( mode == "packetLength" ) {
		if( value % 32 != 0 ) {
			if( FP_SERIALDEBUG && Serial ) Serial.println(str);
			return false;
		}
		int packetLength = (value / 32) - 5;
		if( packetLength > 0 && packetLength < 3 ) {
			paramNumber = 6;
			paramValue = packetLength;
		} else {
			return false;
		}
	} else {
		if( FP_SERIALDEBUG && Serial ) Serial.println(str);
		return false;
	}
	uint8_t result = sendData(1, FP_SYSTEMPARAMSET, 0, paramNumber, paramValue );
	str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: Reads the fp currently used parameter values
	@ arguments : none
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::readSystemParam() {
	uint8_t result = sendData(2, FP_SYSTEMPARAMREAD);
	status_reg = ((uint16_t)fp_content[1] << 8) | fp_content[2];
	system_id = ((uint16_t)fp_content[3] << 8) | fp_content[4];
	capacity = ((uint16_t)fp_content[5] << 8) | fp_content[6];
	security_level = ((uint16_t)fp_content[7] << 8) | fp_content[8];
	deviceAddress = ((uint32_t)fp_content[9] << 24) | ((uint32_t)fp_content[10] << 16) |
				  ((uint32_t)fp_content[11] << 8) | (uint32_t)fp_content[12];
	packet_length = ((uint16_t)fp_content[13] << 8) | fp_content[14];
	baud_rate = (((uint16_t)fp_content[15] << 8) | fp_content[16]);
	
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) 
		Serial.println(str);
	else
		systemParamRead = true;
	if( str == "" && FP_SERIALDEBUG && Serial ) {
		Serial.println("====================================================");
		Serial.println("System Parameters");
		Serial.print("Status Register => 0x");
		Serial.println(status_reg, HEX);
		Serial.print("System Id       => 0x");
		Serial.println(system_id, HEX);
		Serial.print("Capacity        => ");
		Serial.println(capacity, DEC);
		Serial.print("Security Level  => ");
		Serial.println(security_level, DEC);
		Serial.print("Device Address  => ");
		Serial.println(deviceAddress, HEX);
		int packetSize = pow(2, 5 + (int)packet_length);
		Serial.print("Packet Length   => ");
		Serial.println(packetSize, DEC);
		Serial.print("Baud Rate       => ");
		Serial.println((int)baud_rate * 9600);
		Serial.println("====================================================");
	}
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: gets the total count of template saved in the fp library
	@ arguments : none
	@ returns templateCount if no problem encountered otherwise -1
*/
int R307_Fingerprint::getTemplateCount() {
	uint8_t result = sendData(2, FP_TEMPLATECOUNT);
	templateCount = 0;
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) {
		Serial.println(str);
		return -1;
	}
	if( str == "" ) {
		templateCount = (int)((uint16_t)fp_content[0] << 8) | fp_content[1];
		if(FP_SERIALDEBUG && Serial) Serial.println("Template Count => " + String(templateCount));
	}
	return templateCount;
}
//=====================================================================================
/*
	@ description: generate fingerprint image based on the currently
				   placed finger in the fp sensor and saves it in
				   the image buffer of fp
	@ arguments : none
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::generateFpImage() {
	uint8_t result = sendData(2, FP_IMAGEGENERATE);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	else
		createdImageBuffer = true;
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: extracts the fingerprint image stored in the fp image buffer
	@ arguments : none
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::downloadFpImage() {
	// TODO: Find a way to convert the additional packet to bmp
	if( !createdImageBuffer ) {
		if( FP_SERIALDEBUG && Serial ) Serial.println(errorCodeDictionary(FP_FUNCTIONREQUIREMENTNOTMET));
		return false;
	}
	uint8_t result = sendData(2, FP_IMAGEDOWNLOAD);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	if( str == "" ) receiveAdditionalPacket();
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: generate a char file based on the image buffer
				   and stores it in the selected char buffer
	@ arguments :
		bufferId -> determines in what char buffer to store the char file
				   set the value to 1 to use charBuffer1 and any other
				   value for charBuffer2
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::generateFpChar(int bufferId) {
	if( !createdImageBuffer ) {
		if( FP_SERIALDEBUG && Serial ) Serial.println(errorCodeDictionary(FP_FUNCTIONREQUIREMENTNOTMET));
		return false;
	}
	uint8_t result = sendData(3, FP_IMAGETOCHAR, 0, (uint8_t)bufferId);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	if( result == FP_OK ) {
		if( bufferId == 1 ) createdCharBuffer1 = true;
		else createdCharBuffer2 = true;
	}
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: generates fp template based on charBuffer1 and charBuffer2
				   and is stored back to charBuffer1 and charBuffer2
	@ arguments : none
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::generateFpTemplate() {
	if( !createdCharBuffer1 || !createdCharBuffer2 ) {
		if( FP_SERIALDEBUG && Serial ) Serial.println(errorCodeDictionary(FP_FUNCTIONREQUIREMENTNOTMET));
		return false;
	}
	uint8_t result = sendData(2, FP_TEMPLATEGENERATE);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: extracts the char file stored in the selected charBuffer
	@ arguments :
		bufferId -> determines what charBuffer to extract char file from
				   set it to 1 to use charBuffer1 and any other value
				   for charBuffer2
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::downloadFpChar(int bufferId) {
	if( (bufferId == 1 && !createdCharBuffer1) || (bufferId == 2 && !createdCharBuffer2) ) {
		if( FP_SERIALDEBUG && Serial ) Serial.println(errorCodeDictionary(FP_FUNCTIONREQUIREMENTNOTMET));
		return false;
	}
	uint8_t result = sendData(3, FP_TEMPLATEDOWNLOAD, 0, (uint8_t)bufferId);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	if( str == "" ) receiveAdditionalPacket();
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: extracts the char file stored in the selected charBuffer
	@ arguments :
		bufferId -> determines what charBuffer to extract char file from
				   set it to 1 to use charBuffer1 and any other value
				   for charBuffer2
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::storeFpTemplate(int pageId, int bufferId) {
	if( (bufferId == 1 && !createdCharBuffer1) || (bufferId == 2 && !createdCharBuffer2) ) {
		if( FP_SERIALDEBUG && Serial ) Serial.println(errorCodeDictionary(FP_FUNCTIONREQUIREMENTNOTMET));
		return false;
	}
	uint8_t result = sendData(4, FP_TEMPLATESTORE, 0, 0, (uint8_t)bufferId, (uint16_t)pageId);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: loads template at the specified location (pageId) of fp library
				   to charBuffer1 or charBuffer2
	@ arguments : none
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::loadFpTemplate(int pageId, int bufferId) {
	uint8_t result = sendData(4, FP_TEMPLATELOAD, 0, 0, (uint8_t)bufferId, (uint16_t)pageId);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: Deletes a segment (N) of templates in fp library starting from
				   the specified pageId indicated
	@ arguments :
		pageId -> starting location in the fp library
		numberOfTemplatesToDelete -> the number of template to delete starting from pageId
									 defaults at 1 template only
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::deleteFpTemplate(int pageId, int numberOfTemplatesToDelete) {
	uint8_t result = sendData(4, FP_TEMPLATEDELETE, 0, 0, (uint8_t)pageId, (uint16_t)numberOfTemplatesToDelete);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: Empties / clears the fp library of all the templates
	@ arguments : none
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::emptyFpLibrary() {
	uint8_t result = sendData(2, FP_LIBRARYCLEAR);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: Matches if Template stored in CharBuffer1 and CharBuffer2 
				   and determines the matching score of the templates.
	@ arguments : none
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::matchFpCharBuffers() {
	if( !createdCharBuffer1 || !createdCharBuffer2 ) {
		if( FP_SERIALDEBUG && Serial ) Serial.println(errorCodeDictionary(FP_FUNCTIONREQUIREMENTNOTMET));
		return false;
	}
	uint8_t result = sendData(2, FP_TEMPLATEMATCHING);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	return result == FP_OK;
}
//=====================================================================================
/*
	@ description: Searches whole fp Library for the template that matches the one stored
				   in charBuffer1 or charBuffer2
	@ arguments : none
	@ returns true if no problem encountered otherwise false
*/
boolean R307_Fingerprint::fpSearch(int bufferId) {
	// TODO: change the returned value to pageId if no problems encountered
	if( !systemParamRead ) {
		if( FP_SERIALDEBUG && Serial ) Serial.println(errorCodeDictionary(FP_FUNCTIONREQUIREMENTNOTMET));
		return false;
	}
	uint8_t result = sendData(6, FP_FINGERSEARCH, 0, 0, (uint8_t)bufferId, 0x00, capacity);
	String str = errorCodeDictionary(result);
	if( str != "" && FP_SERIALDEBUG && Serial ) Serial.println(str);
	return result == FP_OK;
}
//=====================================================================================
uint8_t R307_Fingerprint::sendData(int mode, uint8_t ic, uint32_t param1,
											 uint8_t param2, uint8_t param3,
											 uint16_t param4, uint16_t param5) {
	if (!fpSerial) return FP_RECEIVEPACKAGEFAIL;
	if( mode == 0 ) {
		readPacket(ic,  (uint8_t)(param1 >> 24),
					    (uint8_t)(param1 >> 16),
					    (uint8_t)(param1 >> 8),
					    (uint8_t)(param1 & 0xFF));
	} else if (mode == 1) {
		readPacket(ic, param2, param3);
	} else if (mode == 2) {
		readPacket(ic);
	} else if(mode == 3) {
		readPacket(ic, param2);
	} else if(mode == 4) {
		readPacket(ic, param3, (uint8_t)(param4 >> 8), (uint8_t)(param4 & 0xFF));
	} else if(mode == 5) {
		readPacket(ic, (uint8_t)(param4 >> 8), (uint8_t)(param4 & 0xFF),
					   (uint8_t)(param5 >> 8), (uint8_t)(param5 & 0xFF));
	} else if(mode == 6) {
		readPacket(ic, param3, (uint8_t)(param4 >> 8), (uint8_t)(param4 & 0xFF),
					           (uint8_t)(param5 >> 8), (uint8_t)(param5 & 0xFF));
	}
	return FP_CODECRASH;
}
//=====================================================================================
void R307_Fingerprint::sendPacket( const R307_fp_packet &packet ) {
	if( !fpSerial ) return;
	
	uint16_t dataPacket_length = packet.cmd_length + 2;
	uint16_t checksum = (dataPacket_length >> 8) + (dataPacket_length & 0xFF) + packet.cmd_type;
	
	fpSerial->write((uint8_t)(packet.cmd_header >> 8));
	fpSerial->write((uint8_t)(packet.cmd_header) & 0xFF);
	for( int a = 0; a < 4; a++ ) {
		fpSerial->write(packet.cmd_address[a]);
	}
	fpSerial->write(packet.cmd_type);
	fpSerial->write((uint8_t)(dataPacket_length >> 8));
	fpSerial->write((uint8_t)(dataPacket_length & 0xFF));
	for(uint8_t b = 0; b < packet.cmd_length; b++) {
		fpSerial->write(packet.cmd_data[b]);
		checksum += packet.cmd_data[b];
	}
	fpSerial->write((uint8_t)(checksum >> 8));
	fpSerial->write((uint8_t)(checksum & 0xFF));
	
	if( FP_SERIALDEBUG && Serial ) {
		Serial.println("====================================================");
		Serial.println("Packet that was sent (In Hex).");
		Serial.println("Header   Module Address   PID   Length   IC   Content");
		Serial.print(" ");
		Serial.print(hexToString((uint8_t)(packet.cmd_header >> 8)));
		Serial.print(hexToString((uint8_t)(packet.cmd_header) & 0xFF));
		Serial.print("       ");
		for( int a = 0; a < 4; a++ ) {
			Serial.print(hexToString(packet.cmd_address[a]));
		}
		Serial.print("      ");
		Serial.print(hexToString(packet.cmd_type));
		Serial.print("     ");
		Serial.print(hexToString((uint8_t)(dataPacket_length >> 8)));
		Serial.print(hexToString((uint8_t)(dataPacket_length & 0xFF)));
		Serial.print("    ");
		for( int a = 0; a < packet.cmd_length; a++ ) {
			if( a == 0 )
				Serial.print(hexToString(packet.cmd_data[a]) + "   ");
			else
				Serial.print(hexToString(packet.cmd_data[a]));
		}
		Serial.println("");
		Serial.print("Checksum -> ");
		Serial.print(hexToString((uint8_t)(checksum >> 8)));
		Serial.println(hexToString((uint8_t)(checksum & 0xFF)));
		Serial.println("====================================================");
	}
	return;
}
//=====================================================================================
uint8_t R307_Fingerprint::receivePacket( R307_fp_packet *packet, uint16_t timeout) {
	uint8_t receivedBytes;
	uint16_t idx = 0, timer = 0;
	String col1 = "Header   Module Address   PID   Length   CC   Content";
	String col2 = "";
	String col3 = "Checksum -> ";
	contentByteCounter = 0;
	
	
	while(true) {
		while (!fpSerial->available()) {
		  delay(1);
		  timer++;
		  if (timer >= timeout) {
			return FP_RECEIVETIMEOUT;
		  }
		}
		receivedBytes = fpSerial->read();
		switch(idx) {
			case 0:
				col2 += " " + hexToString(receivedBytes);
				if(receivedBytes != (FP_HEADER >> 8))
					continue;
				packet->cmd_header = (uint16_t)receivedBytes << 8;
				break;
			case 1:
				packet->cmd_header |= receivedBytes;
				if( packet->cmd_header != FP_HEADER )
					return FP_BADRECEIVEDPACKET;
				col2 += hexToString(receivedBytes) + "    ";
				break;
			case 2:
				col2 += "   ";
			case 3:
			case 4:
			case 5:
				packet->cmd_address[idx - 2] = receivedBytes;
				col2 += hexToString(receivedBytes);
				break;
			case 6:
				packet->cmd_type = receivedBytes;
				col2 += "      " + hexToString(receivedBytes) + "    ";
				break;
			case 7:
				packet->cmd_length = (uint16_t)receivedBytes << 8;
				col2 += " " + hexToString(receivedBytes);
				break;
			case 8:
				packet->cmd_length |= receivedBytes;
				col2 += hexToString(receivedBytes) + "    ";
				break;
			default:
				packet->cmd_data[idx - 9] = receivedBytes;
				if( packet->cmd_length + 8 - idx <= 1 )
					col3 += hexToString(receivedBytes);
				else {
					if( idx == 9 )
						col2 += hexToString(receivedBytes) + "   ";
					else
						col2 += hexToString(receivedBytes);
					if( idx >= 9 && contentByteCounter < 256 ) {
						fp_content[contentByteCounter] = receivedBytes;
						contentByteCounter++;
					}
				}
				if((idx - 8) == packet->cmd_length  ) {
					if( FP_SERIALDEBUG && Serial ) {
						Serial.println("====================================================");
						Serial.println("Received packet");
						Serial.println(col1);
						Serial.println(col2);
						Serial.println(col3);
						Serial.println("====================================================");
					}
					return fp_content[0];
				}
				break;
		}
		idx++;
	}
	return FP_CODECRASH;
}
//=====================================================================================
//*******=======___Private Methods___=======*******//
//=====================================================================================
uint8_t R307_Fingerprint::receiveAdditionalPacket( uint16_t timeout ) {
	uint16_t idx = 0, timer = 0;
	uint8_t receivedBytes;
	while(true) {
		while (!fpSerial->available()) {
		  delay(1);
		  timer++;
		  if (timer >= timeout) {
			return FP_RECEIVETIMEOUT;
		  }
		}
		receivedBytes = fpSerial->read();
		if( idx == 0 ) {
			if( FP_SERIALDEBUG && Serial ) Serial.println("Additional Packet:");
		}
		if( FP_SERIALDEBUG && Serial ) Serial.print(hexToString(receivedBytes));
		idx++;
		if( !fpSerial->available() ) {
			if( FP_SERIALDEBUG && Serial ) Serial.println("");
			return FP_OK;
		}
	}
}
//=====================================================================================
String R307_Fingerprint::hexToString( uint8_t value ) {
	String hexa = String(value, HEX);
	if( value < 16) {
		hexa = "0" + hexa;
	}
	return hexa;
}
//=====================================================================================
String R307_Fingerprint::errorCodeDictionary(uint8_t errorCode) {
	String multipleMeaningError = "Failed to generate character file due to ";
	String multipleMeaningError2 = "Failed to generate second character file due to ";
	switch (errorCode) {
		case FP_OK:
			return "";
		case FP_RECEIVEPACKAGEFAIL:
			return "Failed when receiving packet";
		case FP_NOFINGER_A:
			return "No Fingers placed on the sensor";
		case FP_ENROLLFINGERFAIL:
			return"Failed to enroll finger";
		case FP_GENERATECHARFAIL_A:
			return multipleMeaningError + "over-disorderly fingerprint";
		case FP_GENERATECHARFAIL_B:
			return multipleMeaningError + "over-wet fingerprint";
		case FP_GENERATECHARFAIL_C:
			return multipleMeaningError + "over-disorderly fingerprint";
		case FP_GENERATECHARFAIL_D:
			return multipleMeaningError + "lackness of char pts or over-smallness of fingerprint";
		case FP_FINGERSMISMATCH:
			return "Fingers doesn't match";
		case FP_FINGERMATCHFAIL:
			return "Failed to find finger match";
		case FP_CHARCOMBINEFAIL:
			return "Failed to combine the generated char files";
		case FP_BADLOCATION:
			return "PageID is beyond the Finger Library";
		case FP_TEMPLATEREADFAIL:
			return "Failed to read template";
		case FP_TEMPLATEUPLOADFAIL:
			return "Failed to upload the template generated";
		case FP_MODULEDATARECEIVEFAIL:
			return "Module can't receive the data packages";
		case FP_IMAGEUPLOADFAIL:
			return "Failed to upload image to upper computer";
		case FP_TEMPLATEDELETEFAIL:
			return "Faield to delete template";
		case FP_FINGERLIBRARYCLEARFAIL:
			return "Failed to clear Finger Library";
		case FP_PASSWORDFAIL:
			return "Fingerprint Password given is wrong";
		case FP_GENERATEIMAGEFAIL:
			return "Failed to generate the image for the lackness of valid primary image";
		case FP_FLASHWRITEFAIL:
			return "Failed when writing to Finger Flash";
		case FP_NODEFINITION:
			return "Unknown Error";
		case FP_INVALIDREGISTERNO:
			return "Invalid Register Number";
		case FP_INVALIDREGISTERCONFIG:
			return "Invalid Config of Register";
		case FP_WRONGNOTEPADPAGE:
			return "Notepad Page Number is Incorrect";
		case FP_COMMUNICATIONFAIL:
			return "Failed to communicate with the Fingerprint Sensor";
		case FP_NOFINGER2:
			return "No Finger sensor when second time scanning the finger";
		case FP_ENROLLFINGERFAIL2:
			return "Failed to enroll the second time scanned finger ";
		case FP_GENERATECHARFAIL2_A:
			return multipleMeaningError2 + "lackness of char pts or over-smallness of fingerprint";
		case FP_GENERATECHARFAIL2_B:
			return multipleMeaningError2 + "over-disorderly fingerprint";
		case FP_ALREADYEXISTS:
			return "Scanned Finger already in the Finger Library";
		case FP_BADRECEIVEDPACKET:
			return "Received Packets is altered, different or corrupted";
		case FP_RECEIVETIMEOUT:
			return "Timeout reached when waiting for the Fingerprint Sensor to send its reply";
		case FP_CODECRASH:
			return "The R307_Fingerprint library was modified.";
		case FP_FUNCTIONREQUIREMENTNOTMET:
			return "The function requirement are not met.";
		default:
			return "unknown error code => " + String(errorCode);
	}
}