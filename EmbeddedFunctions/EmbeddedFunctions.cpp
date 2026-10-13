#include "EmbeddedFunctions.h"

#include <cstdint>

EmbeddedFunctions::EmbeddedFunctions() {}

EmbeddedFunctions::~EmbeddedFunctions() {}

/**
* Open a connection to a Galil Controller.
*
* @param address Null-terminated address string. Use direct connection (-d) to connect to hardware or simulator (e.g., "192.168.0.120 -d").
* @param port Integer value for the port to connect to.
*
* @throws error if one occurs.
*/

void EmbeddedFunctions::GOpen(String^ address, const int port) {

	// strip ' -d' from string (192.168.0.120 -d -> 192.168.0.120)
	//String^ host = address->Split(' ')[0];

	GalilClient = gcnew TcpClient(address, port);
	GalilStream = GalilClient->GetStream();

	// Set read timeout to avoid reading a missing reply indefinitely
	GalilStream->ReadTimeout = 1000;
}

/**
* Closes a connection to a Galil Controller.
	`GClose()` should be called whenever a program is finished with a controller. This includes when a program closes. A rule of thumb is that for every `GOpen()` call on a given connection, a `GClose()` call should be found on every code path. Failing to call GClose() may cause controller resources to not be released or can hang the process if there are outstanding asynchronous operations. The latter can occur, for example, if a call to GRead() times out and the process exits without calling GClose(). In this case, GRead() still has an outstanding asynchronous read pending. GClose() will terminate this operation allowing the process to exit correctly.
*
*
* @throws error if one occurs.
*/
void EmbeddedFunctions::GClose() {

	if (GalilStream != nullptr) {
		GalilStream->Close();
		GalilStream = nullptr;
	}

	if (GalilClient != nullptr) {
		GalilClient->Close();
		GalilClient = nullptr;
	}
}

/**
* Performs a *command-and-response* transaction on the connection.
* IMPORTANT: Commands being sent to the galil should be in the form of strings based on the command reference (uploaded to Moodle). You should choose commands appropriate for a given task and avoid those that rely on logical expressions. Commands chosen should send data directly to the output or read directly from the inputs.
* IMPORTANT: If the command string supplied is not terminated by a semicolon, you should append one.
*
* @param command Null-terminated command string to send to the controller.
*
* @return The reponse from the Galil.
* @throws error if one occurs.
*/
String^ EmbeddedFunctions::GCommand(String^ command) { 
	
	// format string to end with ; 
	//TODO: test and add \n, referring to #77 post on edstem
	// #76: "the input string might be missing the carriage return so you need to handle it properly"
	if (!command->EndsWith(";")) {
		command += ";";
	}

	array<uint8_t>^ RecvData = gcnew array<uint8_t>(2048);

	// Format and send command
	array<uint8_t>^ SendData = Text::Encoding::ASCII->GetBytes(command);
	GalilStream->Write(SendData, 0, SendData->Length);

	// read and return output
	int resLength = GalilStream->Read(RecvData, 0, RecvData->Length);
	String^ res = Text::Encoding::ASCII->GetString(RecvData, 0, resLength);
	
	// if response contains ? to indicate an invalid command, print out the message
	if (res->Contains("?")) {
		Console::WriteLine("Galil error: '{0}'", res);
	}

	return res;
}

