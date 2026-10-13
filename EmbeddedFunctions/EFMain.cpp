#include "EmbeddedFunctions.h"

int main(void) {

	EmbeddedFunctions^ funcs = gcnew EmbeddedFunctions();

	funcs->GOpen("127.0.0.1", 26000);

	funcs->GCommand("SB 1");

	funcs->GClose();
	



	return 0;
}