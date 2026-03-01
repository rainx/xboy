#pragma once

#include <cstdint>
#include <string>

namespace test_runner {

enum class TestResult { Passed, Failed, Timeout, InfiniteLoop };

struct TestRunResult {
  TestResult result;
  std::string serial_output;
  uint64_t cycles_run;
};

class HeadlessRunner {
public:
  explicit HeadlessRunner(const std::string &rom_path);

  // Run emulation headless up to max_cycles. Returns result with serial output.
  TestRunResult run(uint64_t max_cycles = 100'000'000);

private:
  std::string rom_path_;
};

} // namespace test_runner
