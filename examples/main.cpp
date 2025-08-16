#include <CORA/CORA.h>
#include <CORA/CORA_problem.h>
#include <CORA/CORA_types.h>
#include <CORA/pyfg_text_parser.h>

#ifdef GPERFTOOLS
#include <gperftools/profiler.h>
#endif


void exportCertificateResultsSingleCSV(
    const CORA::CoraResult & R,
    const std::string& filename)
{
  std::ofstream out(filename);

  // 1) Export SDPval vector entries
  for (size_t k = 0; k < R.SDPvalVector.size(); ++k) {
    out << "SDPval," << k << ",," << R.SDPvalVector[k] << "\n";
  }

  // 2) Export gradnorm vector entries
  for (size_t k = 0; k < R.gradnormVector.size(); ++k) {
    out << "gradnorm," << k << ",," << R.gradnormVector[k] << "\n";
  }

  // 3) Export scalar fields
  out << "startingRank : "         << R.rank_iters.front()         << "\n";
  out << "endingRank : "           << R.rank_iters.back()           << "\n";

  // 4) Export initialization_time vector entries
  out << "initialization_time : " << R.initialization_time << "\n";

  // 5) Export elapsed_optimization_times vector entries
  double FinalOptTime = 0;
  for (size_t k = 0; k < R.elapsed_optimization_times.size(); ++k) {
    out << "elapsed_optimization_times," << k << ","
        << R.elapsed_optimization_times[k].back() << "\n";
    FinalOptTime += R.elapsed_optimization_times[k].back();
  }

  // 6) Export verification_times vector entries
  for (size_t k = 0; k < R.verification_times.size(); ++k) {
    out << "verification_times," << k << ",,"
        << R.verification_times[k] << "\n";
  }

  out << "Final optimization time : " << FinalOptTime << "\n";
  out << "Final total computation time : " << R.total_computation_time << "\n";

  std::cout << "All fields exported to \"" << filename << "\"\n";
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " [input .g2o file]"  << argv[1] << " [output iterations info file]" << std::endl;
    exit(1);
  }

  CORA::Problem problem = CORA::parsePyfgTextToProblem(argv[1]);

  std::string outputPath = argv[2];
  auto initialization_start = Stopwatch::tick();
  problem.updateProblemData();

#ifdef GPERFTOOLS
  ProfilerStart("cora.prof");
#endif
  CORA::Matrix x0 = problem.getRandomInitialGuess();
  auto initialization_time = Stopwatch::tock(initialization_start);
  int max_rank = 10;
  bool verbose = true;
  bool log_iterates = true;
  bool show_iterates = true;

  CORA::CoraResult results = CORA::solveCORA(problem, x0, max_rank, verbose, log_iterates, show_iterates);
  results.initialization_time = initialization_time;
  results.total_computation_time = results.total_computation_time + initialization_time;

//  CORA::Matrix aligned_soln = problem.alignEstimateToOrigin(results.Yopt);

  exportCertificateResultsSingleCSV(results, outputPath);

  // std::cout << "Solution: " << std::endl;
  // std::cout << aligned_soln << std::endl;

#ifdef GPERFTOOLS
  ProfilerStop();
#endif
}
