#include "build_grammar.hpp"
#include "build_lexer.hpp"
#include "semantic/sema.hpp"
#include "utils.hpp"

#include <fstream>
#include <iostream>
#include <vector>

int main(const int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>"
                  << " [-o <output_file>]"
                  << " [-O <optimization_level>]"
                  << " [-- <args_passed_to_clang>]" << std::endl;
        return 1;
    }

    std::vector<std::string> args(argv + 1, argv + argc);
    std::string input_file = args[0];
    std::string output_arg;
    std::string optimize_arg;
    std::string args_passed_to_clang;

    if (const auto output_it = std::ranges::find(args, "-o");
        output_it != args.end() && std::next(output_it) != args.end()) {
        output_arg = "-o " + *std::next(output_it);
    }

    if (const auto optimize_it = std::ranges::find(args, "-O");
        optimize_it != args.end() && std::next(optimize_it) != args.end()) {
        optimize_arg = "-O" + *std::next(optimize_it);
    }

    if (optimize_arg.empty()) {
        for (auto&& arg : args) {
            if (arg.starts_with("-O")) {
                optimize_arg = arg;
                break;
            }
        }
    }

    if (const auto clang_it = std::ranges::find(args, "--");
        clang_it != args.end() && std::next(clang_it) != args.end()) {
        args_passed_to_clang = utils::join(std::vector(std::next(clang_it), args.end()), " ");
    }

    std::ofstream il("tmp.il");

    std::ifstream ifs(input_file);

    std::string input;
    std::string tmp;
    while (std::getline(ifs, tmp)) {
        input += tmp;
        input += '\n';
    }

    auto tokens = lex(input);
    auto prods = build_grammar();
    auto parser = semantic::sema<grammar::LR1>(prods, il);
    parser.build();
    parser.parse(tokens);

    auto tree = std::static_pointer_cast<semantic::sema_tree>(parser.get_tree());
    auto env = tree->calc();

    for (const auto& err : env.errors) {
        std::cerr << err << std::endl;
    }

    if (!env.errors.empty()) {
        std::cerr << "errors occurred, not generating output." << std::endl;
        return 1;
    }

    // call opt to optimize
    std::string opt_cmd = std::string{"opt -S "} + optimize_arg + " -o tmp.opt.ll tmp.il";
    if (system(opt_cmd.c_str()) != 0) {
        std::cerr << "opt failed." << std::endl;
        return 1;
    }

    // call clang to generate executable
    std::string clang_cmd = std::string{"clang "} + optimize_arg + " " + output_arg + " tmp.opt.ll " + args_passed_to_clang;

    if (system(clang_cmd.c_str()) != 0) {
        std::cerr << "clang failed." << std::endl;
        return 1;
    }

    // del tmp files
    std::remove("tmp.il");
    std::remove("tmp.opt.ll");

    return 0;
}
