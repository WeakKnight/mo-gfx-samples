#include "app.h"
#include "string_utils.h"
#ifdef _WIN32
#include <direct.h>
#else
#include<unistd.h>  
#endif
#include <spdlog/spdlog.h>

int main(int, char** args)
{
	auto path = args[0];


#ifdef _WIN32
	auto tokens = StringUtils::Split(path, "\\");

	std::string environmentPath = "";

	for (int i = 0; i < tokens.size() - 1; i++)
	{
		environmentPath += (tokens[i] + "\\");
	}
	environmentPath += "assets\\";

	_chdir(environmentPath.c_str());
#else
	auto tokens = StringUtils::Split(path, "/");
	for (int i = 0; i < tokens.size() - 1; i++)
	{
		environmentPath += (tokens[i] + "/");
	}
	environmentPath += "assets/";

	chdir(environmentPath.c_str());
#endif

	spdlog::info("Environment Path Is {}", environmentPath);

    App app = App();
    app.Run();

    return 0;
}