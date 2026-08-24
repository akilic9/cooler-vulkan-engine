#include <cstdlib>
#include <iostream>
#include "CVEApplication.h"

int main()
{
	try
	{
		CVEApplication app;
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		system("PAUSE");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}