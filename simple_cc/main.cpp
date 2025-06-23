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
                  << " [--keep](keep intermediate files)"
                  << " [-- <args_passed_to_clang>]" << std::endl;
        return 1;
    }

    std::vector<std::string> args(argv + 1, argv + argc);
    std::string input_file = args[0];
    std::string output_arg;
    std::string optimize_arg;
    std::string args_passed_to_clang;
    bool keep = false;

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

    if (std::ranges::find(args, "--keep") != args.end()) {
        keep = true;
    }

    std::string il_name = std::string{"./"} + input_file + ".ll";
    std::string opt_name = std::string{"./"} + input_file + ".opt.ll";
    std::ofstream il(il_name);
    std::ifstream ifs(input_file);

    std::string input;
    std::string tmp;
    while (std::getline(ifs, tmp)) {
        input += tmp;
        input += '\n';
    }

    ifs.close();

    auto tokens = lex(input);
    auto prods = build_grammar();
    auto parser = semantic::sema<grammar::LR1>(prods, il);
    parser.build();
    parser.parse(tokens);

    auto tree = std::static_pointer_cast<semantic::sema_tree>(parser.get_tree());
    auto env = tree->calc();

    il.close();

    for (const auto& err : env.errors) {
        std::cerr << err << std::endl;
    }

    if (!env.errors.empty()) {
        std::cerr << "errors occurred, not generating output." << std::endl;
        return 1;
    }

    // call opt to optimize
    std::string opt_cmd = std::string{"opt -S "} + optimize_arg + " -o " + opt_name + " " + il_name;
    if (system(opt_cmd.c_str()) != 0) {
        std::cerr << "opt failed." << std::endl;
        return 1;
    }

    // call clang to generate executable
    std::string clang_cmd = std::string{"clang "} + optimize_arg + " " + output_arg + " " + opt_name + " " + args_passed_to_clang;

    if (system(clang_cmd.c_str()) != 0) {
        std::cerr << "clang failed." << std::endl;
        return 1;
    }

    if (!keep) {
        std::remove(il_name.c_str());
        std::remove(opt_name.c_str());
    }

    return 0;
}
