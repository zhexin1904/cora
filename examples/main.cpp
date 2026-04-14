#include <CORA/CORA.h>
#include <CORA/CORA_problem.h>
#include <CORA/CORA_types.h>
#include <CORA/CORA_utils.h>
#include <CORA/pyfg_text_parser.h>

#ifdef GPERFTOOLS
#include <gperftools/profiler.h>
#endif


void exportCertificateResultsSingleCSV(
    const CORA::CoraResult & R,
    const std::string& filename)
{
  std::ofstream out(filename);

  out << "field,index,value\n";

  // 1. SDPval for each rank level
  size_t n_iters = R.function_values.size();
  for (size_t i = 0; i < n_iters; ++i) {
    if (!R.function_values[i].empty()) {
      out << "SDPval," << i << "," << R.function_values[i].back() << "\n";
    }
  }

  // 2. gradnorm,0,value (showing the final gradient norm as in the example)
  out << "gradnorm,0," << R.gradnorm << "\n";

  // 3. Summary entries
  out << "startingRank,0," << (R.rank_iters.empty() ? 0 : R.rank_iters.front()) << "\n";
  out << "endingRank,0," << (R.rank_iters.empty() ? 0 : *std::max_element(R.rank_iters.begin(), R.rank_iters.end())) << "\n";
  out << "initialization_time,0," << R.initialization_time << "\n";

  // 4. Group all elapsed_optimization_times
  for (size_t i = 0; i < R.elapsed_optimization_times.size(); ++i) {
    if (!R.elapsed_optimization_times[i].empty()) {
      out << "elapsed_optimization_times," << i << "," << R.elapsed_optimization_times[i].back() << "\n";
    }
  }

  // 5. Group all verification_times
  for (size_t i = 0; i < R.verification_times.size(); ++i) {
    out << "verification_times," << i << "," << R.verification_times[i] << "\n";
  }

  // 6. Final times
  double FinalOptTime = 0;
  for (const auto& times : R.elapsed_optimization_times) {
    if (!times.empty()) {
      FinalOptTime += times.back();
    }
  }
  out << "FinalOptimizationTime,0," << FinalOptTime << "\n";
  out << "FinalTotalTime,0," << R.total_computation_time << "\n";

  std::cout << "All fields exported to \"" << filename << "\"\n";
}

int main(int argc, char **argv) {
  if (argc < 3 || argc > 4) {
    std::cout << "Usage: " << argv[0] << " [input .g2o file] [output iterations info file] [optional: output .g2o trajectory file]" << std::endl;
    exit(1);
  }

  CORA::Problem problem = CORA::parsePyfgTextToProblem(argv[1]);

  std::string outputPath = argv[2];
  std::string g2oOutputPath = (argc == 4) ? argv[3] : "";
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

  // Example of how to adjust the max iterations for each local solver:
  // Optimization::Riemannian::TNTParams<Scalar> params;
  // params.max_iterations = 500;
  // CORA::CoraResult results = CORA::solveCORA(problem, x0, max_rank, verbose, log_iterates, show_iterates, params);
  results.initialization_time = initialization_time;
  results.total_computation_time = results.total_computation_time + initialization_time;

//  CORA::Matrix aligned_soln = problem.alignEstimateToOrigin(results.Yopt);

  exportCertificateResultsSingleCSV(results, outputPath);

  if (!g2oOutputPath.empty()) {
    CORA::Matrix aligned_soln = problem.alignEstimateToOrigin(results.Yopt);
    std::vector<CORA::Symbol> pose_symbols;
    
    // try to get all 'a' symbols (often used in these datasets)
    std::vector<CORA::Symbol> a_symbols = problem.getPoseSymbols('a');
    pose_symbols.insert(pose_symbols.end(), a_symbols.begin(), a_symbols.end());

    // try to get all 'x' symbols
    std::vector<CORA::Symbol> x_symbols = problem.getPoseSymbols('x');
    pose_symbols.insert(pose_symbols.end(), x_symbols.begin(), x_symbols.end());

    // if still empty, get everything from the map
    if (pose_symbols.empty()) {
      for (const auto& [sym, idx] : problem.getPoseSymbolMap()) {
        pose_symbols.push_back(sym);
      }
    }
    std::sort(pose_symbols.begin(), pose_symbols.end());
    
    if (!pose_symbols.empty()) {
      CORA::saveSolnToG20(pose_symbols, problem, aligned_soln, g2oOutputPath);
      std::cout << "Trajectory exported to \"" << g2oOutputPath << "\"\n";
    } else {
      std::cout << "No pose symbols found to export.\n";
    }
  }

  // std::cout << "Solution: " << std::endl;
  // std::cout << aligned_soln << std::endl;

#ifdef GPERFTOOLS
  ProfilerStop();
#endif
}
