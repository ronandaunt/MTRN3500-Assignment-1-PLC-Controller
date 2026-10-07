#include "Galil.h"

const char* ADDRESS = "192.168.0.120 -d";
int main(void) {

	EmbeddedFunctions* funcs = new EmbeddedFunctions(true);
	Galil* galil = new Galil(funcs, ADDRESS);

	galil->DigitalOutput(0b1111111111111111);

	if (galil->CheckSuccessfulWrite()) {
		std::cout << "Write was successful\n";
	}
	else {
		std::cout << "Write failed\n";
	}

	galil->DigitalByteOutput(0, 0b00001111);
	if (galil->CheckSuccessfulWrite()) {
		std::cout << "Write was successful\n";
	}
	else {
		std::cout << "Write failed\n";
	}


	galil->DigitalByteOutput(1, 0b11110000);
	

	if (galil->CheckSuccessfulWrite()) {
		std::cout << "Write was successful\n";
	}
	else {
		std::cout << "Write failed\n";
	}

	galil->DigitalBitOutput(1, 0);


	if (galil->CheckSuccessfulWrite()) {
		std::cout << "Write was successful\n";
	}
	else {
		std::cout << "Write failed\n";
	}


	delete galil;
	delete funcs;
}