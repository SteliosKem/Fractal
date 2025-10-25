#include "Fractal.h"

#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include "Analysis/SemanticAnalyzer.h"
#include "CodeGeneration/CodeGenerator.h"
#include "CodeEmission/IntelCodeEmission.h"

class InputParser{
public:
	InputParser(int &argc, char **argv) {
		for (int i=1; i < argc; ++i)
			this->tokens.push_back(std::string(argv[i]));
	}

	const std::string& getCmdOption(const std::string &option) const {
		std::vector<std::string>::const_iterator itr;
		itr =  std::find(this->tokens.begin(), this->tokens.end(), option);
		if (itr != this->tokens.end() && ++itr != this->tokens.end())
			return *itr;
		static const std::string empty_string("");
		return empty_string;
	}

	bool cmdOptionExists(const std::string &option) const{
		return std::find(this->tokens.begin(), this->tokens.end(), option) != this->tokens.end();
	}

private:
    std::vector<std::string> tokens;
};

int main(int argc, char** argv)
{
	if(argc < 2) {
		std::cout << "Expected arguments. Run Fractal --help to see the correct usage of the command.";
        Fractal::ErrorHandler errorHandler;
        Fractal::Lexer lexer(&errorHandler);
        Fractal::Parser parser(&errorHandler);
        Fractal::SemanticAnalyzer semanticAnalyzer(&errorHandler);
        Fractal::CodeGenerator codeGenerator(&errorHandler);
        Fractal::IntelCodeEmission emitter{};

        if (!lexer.analyze("../../../../Test/src/test.frc")) {
            errorHandler.outputErrors();
            return false;
        }
        lexer.print();

        if (!parser.parse(lexer.getTokenList())) {
            errorHandler.outputErrors();
            return false;
        }

        for (auto& definition : parser.definitions())
            definition->print();
        for (auto& statement : parser.statements())
            statement->print();

        if (!semanticAnalyzer.analyze(&parser.program())) {
            errorHandler.outputWarnings();
            errorHandler.outputErrors();
            return false;
        }
        errorHandler.outputWarnings();

        std::cout << "Analysis Completed" << '\n';

        for (auto instruction : codeGenerator.generate(parser.program(), Fractal::Platform::Win))
            instruction->print();

        std::cout << '\n';

        std::cout << emitter.emit(&codeGenerator.instructions(), codeGenerator.externals(), Fractal::Platform::Win);
		return EXIT_FAILURE;
	}
	InputParser input(argc, argv);
    if(input.cmdOptionExists("-h") || input.cmdOptionExists("--help")){
        std::cout << "Fractal Command Usage:\n"
			<< "-f {file_path}: Compile a single fractal source file.\n"
			<< "-f {file_path} -o {out_path}: Compile a single fractal source file and output the executable to another directory\n"
			<< "create {project_name}: Create a Fractal (Sequence) project in the current directory\n"
			<< "build: Build the Fractal project from the build_config.json file in the current directory\n";
			return EXIT_SUCCESS;
    }
    
	const std::string& projectName = input.getCmdOption("create");
	if(!projectName.empty()) {
		Fractal::createProject(std::filesystem::current_path(), Fractal::Project{projectName, "src", "build", "x86_64-intel-win"});
		return EXIT_SUCCESS;
	}

	if(input.cmdOptionExists("build")) {
		if(Fractal::buildProject(std::filesystem::current_path()))
			return EXIT_SUCCESS;
		return EXIT_FAILURE;
	}
	
}
