#include "Galil.h"

#include <sstream>
#include <iomanip>
using namespace System;

// Default constructor. Initialize variables, open Galil connection and allocate memory.
// Should assign a default embedded functions that works with physical hardware and a 
// default Galil address as described in the assignment spec.
Galil::Galil() : Functions(new EmbeddedFunctions()), ownFunctions(true), g(0) {

	//TODO: USe simulator toggle for EmbeddedFunctions call?

	// open Galil connection: if error in opening connection delete Functions object as we own it
	try {
		openConnection();
	}
	catch (const std::exception& e) {
		delete Functions;
		throw;
	}

}										

// Constructor with EmbeddedFunciton pre-initialised and passed in.
Galil::Galil(EmbeddedFunctions* Funcs, GCStringIn address) : Functions(Funcs), ownFunctions(false), g(0) {
	openConnection(address);
}	

void Galil::openConnection(GCStringIn address) {
	GReturn res = Functions->GOpen(address, &g);

	if (res != G_NO_ERROR) {
		
		// parse error message and throw error
		char msg[256] = {};
		GError(res, msg, sizeof(msg));

		throw std::runtime_error(std::string("GOpen failed: ") + msg);

	}
}

// Copy constructor to copy the state of all elements within the object other.
// It should construct a new EmbeddedFunctions object and open a separate connection
// (i.e., each class will have a unique value of the GCon g). All other data members
// should be transferred.
Galil::Galil(const Galil& other) :Functions(new EmbeddedFunctions()), ownFunctions(true), 
			setPoint(other.setPoint), g(0)  {

	std::copy(std::begin(other.ControlParameters), std::end(other.ControlParameters),
		std::begin(ControlParameters));
	std::copy(std::begin(other.readBuffer), std::end(other.readBuffer),
		std::begin(readBuffer));
	
	// open Galil connection: if error in opening connection delete Functions object as we own it
	try {
		openConnection();
	}
	catch (const std::exception& e) {
		delete Functions;
		throw;
	}
}								

// Default destructor. Deallocate memory and close Galil connection.
Galil::~Galil() {

	Functions->GClose(g);

	if (ownFunctions) {
		delete Functions;
	}


}											

// DIGITAL OUTPUTS

// Write to all 16 bits of digital output, 1 command to the Galil
void Galil::DigitalOutput(uint16_t value) {

	// Bit operations to obtain outputs for low and high banks
	uint8_t valueLowBank = value & 0b11111111;
	uint8_t valueHighBank = value >> 8; // bit shifts 8 to right (like dividing by 2^8)


	std::string command = "OP" + std::to_string(valueLowBank) + "," + std::to_string(valueHighBank) + ";";
	sendCommand(command.c_str());

}

// Sends command and throws error if command fails
void Galil::sendCommand(GCStringIn command) {
	GSize bytesReturned = 0;
	GReturn res = Functions->GCommand(g, command, readBuffer, sizeof(readBuffer), &bytesReturned);

	if (res != G_NO_ERROR) {

		// parse error message and throw error
		char msg[256] = {};
		GError(res, msg, sizeof(msg));

		throw std::runtime_error(std::string("GCommand failed: ") + msg);

	}
}

// Write to one byte, either high or low byte, as specified by user in 'bank'
// 0 = low, 1 = high
void Galil::DigitalByteOutput(bool bank, uint8_t value) {
	// for upper bank, try OP ,value
	// for lower bank, do OPvalue

	std::string command = "OP";

	if (bank == true) {
		command += " ,";
	}

	command += std::to_string(value) + ";";
	sendCommand(command.c_str());
}		


// Write single bit to digital outputs. 'bit' specifies which bit
void Galil::DigitalBitOutput(bool val, uint8_t bit) {
	
	// SB for true, CB for false
	std::string command = val ? "S" : "C";

	command += "B" + std::to_string(bit) + ";";

	sendCommand(command.c_str());

}			


// DIGITAL INPUTS

// Return the 16 bits of input data
// Query the digital inputs of the GALIL, See Galil command library @IN
uint16_t Galil::DigitalInput() {

	uint16_t input = 0;
	for (int i = 0; i < 16; i++) {

		if (DigitalBitInput(i)) {
			// or operand with byte Input and the bit mask
			input |= (1 << i);
		}
	}

	return input;
}	



// Read either high or low byte, as specified by user in 'bank'
// 0 = low, 1 = high
// A bank is one byte (8 bits). The low bank (0) is the first 8
// bits (DI0-DI7) and the high bank (1) is the upper 8 bits
// (DI8-DI15).
uint8_t Galil::DigitalByteInput(bool bank) {

	int startBit = bank ? 8 : 0;

	uint8_t byteInput = 0;
	for (int i = 0; i < 8; i++) {

		if (DigitalBitInput(i + startBit)) {
			byteInput |= (1 << i);
		}
	}

	return byteInput;

}					


// Read single bit from current digital inputs. Above functions
// may use this function
bool Galil::DigitalBitInput(uint8_t bit) {
	std::string command = "MG @IN[" + std::to_string(bit) + "];";
	sendCommand(command.c_str());

	// stod: string to double
	double input = std::stod(readBuffer);

	// return as a bool
	return (input != 0.0);
}						

// Check the string response from the Galil to check that the last
// command executed correctly. 1 = succesful.
// A successful write indicates that a write command (sending a 
// message to the Galil that does not have a response -- e.g., 
// digitalOutput, analogOutput) has completed without errors.
// This should validate some part of the Galil's response (it is 
// up to you how this is completed) but should validly
// differentiate when a write command has been completed 
// successfully.
// This will be called from your main function (do not call it
// within your implementation functions of this Galil class.
bool Galil::CheckSuccessfulWrite() {

	for (int i = 0; readBuffer[i] != '\0' && i < 16; i++) {
		std::cout << int(readBuffer[i]) << " ";

		if (readBuffer[i] == ':') { return true; }
	}

	return false;

}							

// ANALOG FUNCTIONS

// Read Analog channel and return voltage
float Galil::AnalogInput(uint8_t channel) {
	std::string command = "MG@AN[" + std::to_string(channel) + "];";

	sendCommand(command.c_str());

	return std::stof(readBuffer);
}	


// Write to any channel of the Galil, send voltages as
// 2 decimal place in the command string
void Galil::AnalogOutput(uint8_t channel, double voltage) {
	
	// use std::fixed and setprecision to send voltages with 2dp
	std::ostringstream command;
	command << "AO " << channel << "," << std::fixed << std::setprecision(2) << voltage << ";";

	sendCommand(command.str().c_str());

}


// Configure the range of the input channel with
// the desired range code
// Range: 1 -> +/- 5V, 2 -> +/- 10V, 3 -> 0 - 5V, 4 -> 0 - 10V
void Galil::AnalogInputRange(uint8_t channel, uint8_t range) {

	std::string command = "AQ " + std::to_string(channel) + "," + std::to_string(range) + ";";

	sendCommand(command.c_str());

	//todo: check for invalid calls, such as check for invalid channel? or is that passed onto galil

}	

// ENCODER 

// Manually Set the motor encoder value to zero (encoder channel 0)
void Galil::WriteEncoder() {


	// read encoder value for channel 1
	std::string query = "QE 1;";
	sendCommand(query.c_str());
	int channel1 = std::stoi(readBuffer);

	// write 0 to encoder channel 0, preserving channel 1 count

	std::string command = "WE 0," + std::to_string(channel1) + ";";

	sendCommand(command.c_str());
}	

// Read from motor Encoder (encoder channel 0)
int Galil::ReadEncoder() {
	std::string command = "QE 0;";

	sendCommand(command.c_str());

	return std::stoi(readBuffer);
}										

// CONTROL FUNCTIONS

// Set the desired setpoint for control loops, counts or counts/sec
// This should set it within the class not on the actual Galil.
void Galil::setSetPoint(int s) {
	setPoint = s;
}	

// Gets the current setpoint stored in the class
double Galil::getSetPoint() {
	return setPoint;
}	

// Set the proportional gain of the controller used in controlLoop() of Position/SpeedControl
// This should set it within the class not on the actual Galil
void Galil::setKp(double gain) {
	ControlParameters[0] = gain;
}

// Gets the current proportional gain stored in the class
double Galil::getKp() {
	return ControlParameters[0];
}											

// Set the integral gain of the controller used in controlLoop()  of Position/SpeedControl
// This should set it within the class not on the actual Galil.
void Galil::setKi(double gain) {
	ControlParameters[1] = gain;
}								

// Gets the current integral gain stored in the class
double Galil::getKi() {
	return ControlParameters[1];
}	

// Set the derivative gain of the controller used in controlLoop()  of Position/SpeedControl
// This should set it within the class not on the actual Galil.
void Galil::setKd(double gain) {
	ControlParameters[2] = gain;
}								

// Gets the current derivative gain stored in the class
double Galil::getKd() {
	return ControlParameters[2];
}

// OPERATOR OVERLOADS
// TODO: complete this function.
// Operator overload for '<<' operator. So the user can say cout << Galil;
// This function should print out the output of GInfo and GVersion, with
// two newLines after each.
std::ostream& operator<<(std::ostream& output, Galil& galil) {

	// need to send GInfo command from embedded functions object

	//GSize bytesReturned = 0;
	//GReturn res = Functions->GCommand(g, command, readBuffer, sizeof(readBuffer), &bytesReturned);
	GReturn infoRes = galil.Functions->GInfo(galil.g, galil.readBuffer, sizeof(galil.readBuffer) );


	// add that output to stream alongside two newlines
	output << galil.readBuffer << "\n\n";

	// repeat with gversion

	GReturn verRes = galil.Functions->GVersion(galil.readBuffer, sizeof(galil.readBuffer));
	
	output << galil.readBuffer << "\n\n";
	return output;
}

//TODO: Complete this function.
// Copy assignment operator. This acts in the same way as the copy constructor
// (refer above for details).
Galil& Galil::operator=(const Galil& other) {
	
	// early return if copy assignment is applied to same object
	if (this == &other) {
		return *this;
	}

	// Close current objects galil connection
	Functions->GClose(g);
	g = 0;

	if (ownFunctions) { delete Functions; }

	// copy parameters from other and open new galil connection
	std::copy(std::begin(other.ControlParameters), std::end(other.ControlParameters),
		std::begin(ControlParameters));
	std::copy(std::begin(other.readBuffer), std::end(other.readBuffer),
		std::begin(readBuffer));
	setPoint = other.setPoint;

	Functions = new EmbeddedFunctions();
	ownFunctions = true;

	try {
		openConnection();
	}
	catch (const std::exception&) {
		delete Functions;
		ownFunctions = false;
		throw;
	}

	return *this;
}									

