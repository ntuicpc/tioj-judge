#include <fcntl.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <thread>
#include <fstream>
#include <filesystem>

#include <tortellini.hh>
#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>

#include <tioj/logger.h>
#include "paths.h"
#include "tioj/paths.h"
#include "tioj/submission.h"
#include "cpuset.h"
#include "server_io.h"

namespace {

bool to_lock = true;

void SetPinnedCPU(const std::string& cpu_str) {
  if (!CpusetParse(cpu_str.c_str(), &kPinnedCpus, get_nprocs())) {
    throw std::runtime_error("Invalid CPU mask");
  }
}

void ParseConfig(const fs::path& conf_path) {
  std::ifstream fin(conf_path);
  if (!fin) throw std::runtime_error("File not found");
  tortellini::ini ini;
  fin >> ini;
  std::string box_root = ini[""]["box_root"] | "";
  std::string submission_root = ini[""]["submission_root"] | "";
  std::string testdata_root = ini[""]["testdata_root"] | "";
  if (box_root.size()) kBoxRoot = box_root;
  if (submission_root.size()) kSubmissionRoot = submission_root;
  if (testdata_root.size()) kTestdataRoot = testdata_root;
  kMaxParallel = ini[""]["parallel"] | kMaxParallel;
  SetPinnedCPU(ini[""]["pinned_cpus"] | "none");
  kMaxRSS = (ini[""]["max_rss_per_task_mb"] | (kMaxRSS / 1024)) * 1024;
  kMaxOutput = (ini[""]["max_output_per_task_mb"] | (kMaxRSS / 1024)) * 1024;
  kMaxQueue = ini[""]["max_submission_queue_size"] | (kMaxParallel + 2);
  kTimeMultiplier = ini[""]["time_multiplier"] | kTimeMultiplier;
  kTIOJUrl = ini[""]["tioj_url"] | kTIOJUrl;
  kTIOJKey = ini[""]["tioj_key"] | kTIOJKey;
}

void ParseArgs(int argc, char** argv) {
  int verbosity = 0;
  argparse::ArgumentParser parser(
      argc ? argv[0] : "tioj-judge", kVersionCode, argparse::default_arguments::help);
  parser.add_argument("--version")
    .default_value(false)
    .implicit_value(true)
    .help("Show version and exit");
  parser.add_argument("-c", "--config")
    .required().default_value(std::string("/etc/tioj-judge.conf"))
    .help("Path of configuration file");
  parser.add_argument("-v", "--verbose")
    .action([&](const auto &) { ++verbosity; })
    .append().default_value(false).implicit_value(true).nargs(0)
    .help("Verbose level");
  parser.add_argument("-p", "--parallel")
    .scan<'d', int>()
    .help("Number of maximum parallel judge tasks");
  parser.add_argument("-m", "--time-multiplier")
    .scan<'g', double>()
    .help("Ratio of real time to indicated time");
  parser.add_argument("--no-lock")
    .default_value(false)
    .implicit_value(true)
    .help("Not check for other running instances");
  parser.add_argument("--pinned-cpus")
    .help("Comma-separated list of CPUs to pin or simply \"all\" or \"none\"");

  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << parser;
    exit(1);
  }

  if (parser["--version"] == true) {
    std::cout << kVersionCode << std::endl;
    exit(0);
  }
  switch (verbosity) {
    case 0: spdlog::set_level(spdlog::level::warn); break;
    case 1: spdlog::set_level(spdlog::level::info); break;
    default: spdlog::set_level(spdlog::level::debug); break;
  }
  fs::path config_file = parser.get<std::string>("--config");
  try {
    ParseConfig(config_file);
  } catch (const std::runtime_error& err) {
    spdlog::error("Failed to parse configuration file {}: {}", std::string(config_file), err.what());
    exit(1);
  }
  if (auto val = parser.present<int>("--parallel")) {
    kMaxParallel = val.value();
  }
  if (auto val = parser.present<double>("--time-multiplier")) {
    kTimeMultiplier = val.value();
  }
  to_lock = parser["--no-lock"] == false;
  if (auto val = parser.present<std::string>("--pinned-cpus")) {
    try {
      SetPinnedCPU(val.value());
    } catch (const std::runtime_error& err) {
      spdlog::error("{}", err.what());
      exit(1);
    }
  }
  if (CPU_COUNT(&kPinnedCpus) && CPU_COUNT(&kPinnedCpus) < kMaxParallel) {
    spdlog::warn("Parallelism larger than the number of pinned CPUs. Some tasks may not be pinned.");
  }
}

bool LockFile() {
  fs::path lock_file = kTestdataRoot / "lock";
  int fd = open(lock_file.c_str(), O_RDWR | O_CREAT, 0644);
  if (fd < 0) return false;
  struct flock lock{};
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = lock.l_len = 0;
  if (fcntl(fd, F_SETLK, &lock) < 0) return false;
  return true;
}

} // namespace

int main(int argc, char** argv) {
  spdlog::set_pattern("[%t] %+");
  InitLogger();
  ParseArgs(argc, argv);
  if (geteuid() != 0) {
    spdlog::error("Must be run as root.");
    return 1;
  }
  if (to_lock && !LockFile()) {
    spdlog::error("Another judge instance is running.");
    return 1;
  }
  std::thread server_thread(ServerWorkLoop);
  server_thread.detach();
  WorkLoop();
}
