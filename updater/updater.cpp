#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

struct args {
  std::string infile = "";
  std::string outfile = "";
};

static void ConfigureArgs(int argc, char* argv[]){
	std::cout << "configuring args\n";
	std::ofstream out("updater.info");
	for(int i = 0; i < argc; i++){
		std::cout << argv[i] << "\n";
		out << argv[i] << "\n";
	}
}

int main(int argc, char* argv[]){
	std::cout << "Test\n";
	ConfigureArgs(argc, argv);
	return 0;
}