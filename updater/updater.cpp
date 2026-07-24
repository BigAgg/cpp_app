#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <thread>

namespace fs = std::filesystem;

struct args {
  std::string infile = "";
  std::string outfile = "";
  bool startafter = false;
};

static args ConfigureArgs(int argc, char* argv[]){
  if (argc < 2)
    return args();
  args arglist{};
  // Log to file
	std::ofstream out("updater.info");
  // Loop args
	for(int i = 0; i < argc; i++){
    const std::string arg = argv[i];
    // Infile
		if (arg == "--infile") {
      i++;
			if (i >= argc) {
        out << "Missing argument for --infile\n";
        continue;
			}
      const std::string infile = argv[i];
			if (!fs::exists (infile)) {
        out << "Invalid --infile path: " << infile << "\n";
        continue;
			}
      arglist.infile = infile;
    }
    // Outfile
    else if (arg == "--outfile") {
      i++;
      if (i >= argc) {
        out << "Missing argument for --outfile\n";
        continue;
      }
      const std::string outfile = argv[i];
      if (!fs::exists (outfile)) {
        out << "Invalid --outfile path: " << outfile << "\n";
        continue;
      }
      arglist.outfile = outfile;
    }
    // Start after
    else if (arg == "--start") {
      arglist.startafter = true;
    }
	}
  out << "Loaded arglist:\n\tinfile: " << arglist.infile << "\n\toutfile: " << arglist.outfile << "\n\tstart: " << arglist.startafter << "\n";
  out.close();
  return arglist;
}

static bool WaitForWritableFile(const std::string &path, std::chrono::milliseconds retryInterval, std::chrono::seconds timeout) {
  auto start = std::chrono::steady_clock::now();

  while (true) {
    {
      std::ofstream file(path, std::ios::app);
      if (file.is_open())
        return true;
    }

    if (std::chrono::steady_clock::now() - start >= timeout)
      return false;

    std::this_thread::sleep_for(retryInterval);
  }
}

int main(int argc, char* argv[]){
	const args arglist = ConfigureArgs(argc, argv);
  if (arglist.infile.empty() || arglist.outfile.empty())
    return 1;
  std::ofstream out("updater.info", std::ios::app);
  while (WaitForWritableFile (arglist.outfile, std::chrono::milliseconds (100), std::chrono::seconds (1))) {
    if (out)
      out << "Waiting for file to be writable: " << arglist.outfile << "\n";
    std::cout << "Waiting for file to be writable: " << arglist.outfile << "\n";
  }
  fs::copy(arglist.infile, arglist.outfile);
  if (arglist.startafter) {
    if (out)
      out << "Starting " << arglist.outfile << "\n";
    std::cout << "Starting " << arglist.outfile << "\n";
    system(("start " + arglist.outfile).c_str());
  }
	return 0;
}