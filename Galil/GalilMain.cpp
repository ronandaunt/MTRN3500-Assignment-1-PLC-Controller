#include "Galil.h"

int main(void) {
	EmbeddedFunctions func;
	Galil g(&func, "192.168.0.120 -d");
	// code for the task
	return 0;
}