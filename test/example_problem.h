#ifndef EXAMPLE_PROBLEM_H_
#define EXAMPLE_PROBLEM_H_

#include <filesystem>

#include <gtest/gtest.h>
#include <tioj/submission.h>

class ExampleProblem : public ::testing::Test {
 public:
  void SetUp(int problem_id_, int td_num, int max_parallel = 1);
  void SetUp(int problem_id_, const std::vector<std::pair<std::string, std::string>>& tds, int max_parallel = 1);
  void RunAndTeardownSubmission(long id);

  Submission sub;
 protected:
  void TearDown() override;

  int problem_id;
  std::filesystem::path td_path;
};

#endif  // EXAMPLE_PROBLEM_H_
