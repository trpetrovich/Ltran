#include <any>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Value.h>
#include <map>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/Support/CommandLine.h>
/*
template<typename T>
class ProtectedVector {
    private:
        std::vector<T> Real;
    public:
        void push_back(T Value){
            Real.push_back(Value);
        }
        int size(){
            return Real.size();
        }
};*/
/**
 * @brief Interpreter header for the Ltran JIT compiler. (TBWO)
 */
std::string JITHeader = "\e[1;36m[Ltransh]\e[0;37m";

/**
 * @brief Compiler header for messages relating to the Ltran compiler.
 */
std::string CompilerHeader = "\e[1;34m[Ltranc]\e[0;37m";

/**
 * @brief Header for errors and such.
 */
std::string ErrorHeader = "\e[1;31m[ERROR]\e[0;37m";

/**
 * @brief Header for complete operations.
 */
std::string DoneHeader = "\e[1;32m[DONE]\e[0;37m";

/**
 * @brief Header for debug prints.
 */
std::string DebugHeader = "\e[1;35m[DEBUG]\e[0;37m";

/**
 * @brief Global variable storage at compile-time. TBWO!
 */
std::unordered_map<std::string, llvm::GlobalVariable*> Globals = {};

/**
 * @brief Linked library storage populated at compile-time.
 */
std::vector<std::string> LinkedLibraries = {};

/**
 * @brief Included file storage populated at compile-time.
 */
std::vector<std::string> IncludedFiles = {};

/**
 * @brief Context storage for shit outside of main.
 */
llvm::LLVMContext* GlobalContext; 

/**
 * @brief Debug mode - enables verbose printing.
 */
bool DEBUG = true;

/**
 * @brief Debug levels - more verbose the higher it is set.
 */
int DebugLevel = 0;

/**
* @brief This serves to organize all logging prints.
*/
namespace Log {
    /**
    * @brief Regular logging prints.
    * @todo Add option for JIT interface.
    */
    void Print(std::string Message) {
        std::cout << CompilerHeader+" "+Message;
    }

    /**
    * @brief Debug prints that only print based on the level, so we don't need conditionals everywhere with DebugLevel.
    * @todo Add option for JIT interface.
    */
    void DebugPrint(int Level, std::string Message){
        if (DebugLevel >= Level){
            std::cout << DebugHeader+" "+CompilerHeader+" "+Message;
        }
    }

    /**
    * @brief Erroring prints that have an option to kill the program with a given code.
    * @todo Add option for JIT interface.
    */
    void Error(bool Fatal = true, std::string Message = "", int code = -1){
        std::cout << ErrorHeader+" "+CompilerHeader+" "+Message;
        if (Fatal){
            std::exit(code);
        }
    }
};


/**
 * @brief Procedures in C linked to Ltran.
 */
class CLinkedProcedure {
    public:

        /**
        * @brief Type of function. Whoduv thought?
        */
        llvm::FunctionType* Type;

        /**
        * @brief Types of the arguments expected by the function.
        */
        std::vector<llvm::Type*> ArgTypes = {};

        /**
        * @brief Type of the value expected to be returned by the function.
        */
        llvm::Type* ReturnType;

        /**
        * @brief Name of the function, both for identification and usage by LLVM.
        */
        std::string Name;

        /**
        * @brief Basic constructor. 
        * @bug Return type here should never be nullptr.
        */
        CLinkedProcedure(std::vector<llvm::Type*> _ArgTypes, llvm::Type* _ReturnType, std::string _Name){
            ArgTypes = _ArgTypes;
            ReturnType = _ReturnType;
            Name = _Name;
        }
};

/**
* @brief Functions linked through C. 
*/
std::vector<CLinkedProcedure> ExposedFunctions = {};

/**
* @brief Represents objects managed by Ltran. 
*/
class LtranObject {
    
    public:
        /**
        * @brief The actual C++ value represented by the object.
        */
        std::any Content;

        /**
        * @brief Actual type as a string.
        * @details The type of the object, as a string. The possible signatures are as follows:
        * - 'integer'        : A 64 bit unsigned integer.
        * - 'string'         : An unsigned eight bit integer pointer.
        * - 'call'           : A procedure call with a vector of LtranObjects..
        * - 'incompletecall' : A procedure call with a vector of strings that have yet to become objects..
        */
        std::string Signature;

        /**
        * @brief Basic constructor. 
        * @bug Validate signature.
        */
        LtranObject(std::any _Content, std::string _Signature) :
            Content(_Content), Signature(_Signature) {}
};

/**
* @brief Procedures that represent operations in Ltran, like functions in other languages.
*/
class Procedure {
    public:

        /**
        * @brief Names of the arguments passed to the function.
        */
        std::vector<LtranObject> Args = {};

        /**
        * @brief Types of the arguments passed to the function.
        */
        std::vector<llvm::Type*> arg_types = {};

        /**
        * @brief The code executed by the function.
        */
        std::vector<std::vector<LtranObject>> content = {};

        /**
        * @brief The name of the function.
        */
        std::string name = "";

        /**
        * @brief The function's return type.
        */
        llvm::Type* return_type = nullptr;
};

template<typename T>
/**
* @brief A helper function that casts the content of an LtranObject object and then creates an error if it is improper.
* @param Object: The object to have its content cast.
* @return: An object of the requested type.
*/
T ubox(LtranObject Object) {
    if (T* ptr = std::any_cast<T>(&Object.Content)) {
        return *ptr;
    }
    Log::Error(true, "Bad LtranObject unbox performed on object of type signature "+Object.Signature+" and typeid "+typeid(T).name()+". If a regular user is reading this, please report this issue on GitHub with the error message. Sorry :<\n",-3);
    std::exit(-3); // completely unnecessary but it shuts up clangd </3 
}

/**
* @brief A funtion that parses the file into the procedures referenced in the file.
* @param Content: The content of the file to be parsed.
* @return: A vector of all procedures in the current program.
*/
std::vector<Procedure> Parser(std::string Content){
    Log::DebugPrint(1, "Entered parser...\n");
    /**
    * @brief The tokens as std::strings parsed from Content.
    */
    std::vector<std::vector<std::string>> Tokens = {};

    /**
    * @brief The current line being parsed.
    */
    std::string Line;

    /**
    * @brief Used to break down the content per line with getline.
    */
    std::stringstream Stream(Content);

    /**
    * @brief Helper function to break a line into tokens.
    */
    auto ParseLine = [&](std::string Line){
        /**
        * @brief The tokens from the line.
        */
        std::vector<std::string> Tokens;

        /**
        * @brief The current token being built.
        */
        std::string CurrentToken = "";

        /**
        * @brief Whether or not the token we are currently parsing is a string.
        * @details If so, we do not break the token apart by parentheses or whitespace.
        */
        bool InString = false;

        /**
        * @brief The amount of parentheses we are inside. This amount is added to for every opening parenthesis '(' that we pass, and subtraced for every closing parenthesis ')'.
        */
        int Parentheses = 0;
        for (int i = 0; i < Line.size(); ++i){
            switch (Line[i]) {
                case ' ':
                    if (!CurrentToken.empty()){
                        if (!InString && !Parentheses) {
                            Tokens.push_back(CurrentToken);
                            CurrentToken.clear();
                            continue;
                        }
                        CurrentToken += Line[i];
                        break;
                    }
                    break;
                default:
                    CurrentToken += Line[i];
                    break;

                case '\"':
                    CurrentToken += Line[i];
                    if (Line[i-1] == '\\' || Parentheses > 0)
                        break;
                    InString = !InString;
                    break;

                case '(':
                    CurrentToken += Line[i];
                    if (Line[i-1] == '\\' || InString)
                        break;
                    ++Parentheses;
                    break;

                case ')':
                    CurrentToken += Line[i];
                    if (Line[i-1] == '\\' || InString)
                        break;
                    --Parentheses;
                    break;
                case '\\':
                    continue;
            }
            if (CurrentToken.ends_with(';')){
                CurrentToken.erase(CurrentToken.size()-1);
                break;
            }
        }
        if (!CurrentToken.empty()){
            Tokens.push_back(CurrentToken);
        }
        return Tokens;
    };

    while (std::getline(Stream, Line)) {
        auto LineTokens = ParseLine(Line);
        if (!LineTokens.empty()) Tokens.push_back(LineTokens);
        Log::DebugPrint(2, "Parsing line '"+Line+"'...\n");
    }

    /**
    * @brief The tokens that have been turned into LtranObjects.
    */
    std::vector<std::vector<LtranObject>> ParsedTokens = {};

    /**
    * @brief Function that converts a token into an LtranObject.
    */
    auto MakeObject = [&](std::string Token){
        Log::DebugPrint(3, "Making object from token '"+Token+"'...\n");
        if (Token[0] == '\"' && Token[Token.size()-1] == '\"') {
            Token.erase(0, 1);
            Token.erase(Token.size()-1, 1);
            return LtranObject(std::any(Token), "string");
        }
        if (Token[0] == '(' && Token[Token.size()-1] == ')') {
            Token.erase(0, 1);
            Token.erase(Token.size()-1, 1);
            auto IncompleteCallVector = ParseLine(Token);
            return LtranObject(std::any(IncompleteCallVector), "incompletecall");
        }

        try {
            std::stoi(Token);
            return LtranObject(std::any(std::stoi(Token)), "integer");
        } catch (std::exception &e) {
            return LtranObject(std::any(Token), "label");
        }
    };
    for (auto TokenVector : Tokens) {
        std::vector<LtranObject> CurrentVect = {};
        for (auto Token : TokenVector){
            CurrentVect.push_back(MakeObject(Token));
        }

        /**
        * @brief Turn IncompleteCalls into Calls by turning their content from Strings to LtranObjects.
        */
        auto CompleteCall = [&](this const auto& self, LtranObject Object) -> LtranObject {
            if (Object.Signature == "incompletecall"){
                std::vector<LtranObject> VectA = {};
                for (auto Token : ubox<std::vector<std::string>>(Object)) {
                    auto object = MakeObject(Token);
                    if (object.Signature == "incompletecall"){
                        VectA.push_back(self(object));
                    } else {
                        VectA.push_back(object);
                    }
                }
                Object.Content = std::any(VectA);
                Object.Signature = "call";
                return Object;
            }
            return Object;
        };

        for (LtranObject& Object : CurrentVect){
            Object = CompleteCall(Object);
        }
        ParsedTokens.push_back(CurrentVect);
    }
    std::vector<Procedure> procedures = {};
    Procedure current_proc;
    bool inproc = false;
    if (ParsedTokens.empty()){
        Log::Error(true, "No tokens to parse! Compilation terminated.\n", -4);
    }
    for (auto line : ParsedTokens){
        Log::DebugPrint(1, "Iterating through parsed tokens...");
        if (line[0].Signature == "label" && !inproc){
            if (ubox<std::string>(line[0]) == "proc"){
                Procedure proc;
                proc.name = ubox<std::string>(line[1]);
                for (int i=2; i<line.size(); ++i) {
                    if (line[i].Signature == "label" &&
                        ubox<std::string>(line[i]) == "do") break;
                    else if (line[i].Signature == "call") {
                        auto LtranObjectVector = ubox<std::vector<LtranObject>>(line[i]);
                        if (ubox<std::string>(LtranObjectVector[0]) == "setarg"){
                            proc.Args.push_back(LtranObjectVector[1]);
                            std::string x = ubox<std::string>(LtranObjectVector[2]);
                            if (x == "i64")
                                proc.arg_types.push_back(llvm::Type::getInt64Ty(*GlobalContext));
                            else if (x == "string")
                                proc.arg_types.push_back(llvm::PointerType::getUnqual(*GlobalContext));
                        }
                    }
                }
                current_proc = proc;
                inproc = true;
                continue;
            }
        } else if (ubox<std::string>(line[0]) == "corp" && inproc){
            procedures.push_back(current_proc);
            inproc = false;
            continue;
        } else if (ubox<std::string>(line[0]) == "ret" && inproc){
            if (line.size() == 1){
                current_proc.return_type = llvm::Type::getVoidTy(*GlobalContext);
            }
            if (line[1].Signature == "string"){
                current_proc.return_type = llvm::PointerType::getUnqual(*GlobalContext);
            } else if (line[1].Signature == "integer"){
                current_proc.return_type = llvm::Type::getInt64Ty(*GlobalContext);
            } else if (line[1].Signature == "incompletecall"){
                current_proc.return_type = llvm::Type::getInt64Ty(*GlobalContext);
            } 
        } else if (ubox<std::string>(line[0]) == "extern" && !inproc){
            if (ubox<std::string>(line[1]) == "include"){
                IncludedFiles.push_back(ubox<std::string>(line[2]));
            } else if (ubox<std::string>(line[1]) == "link"){
                LinkedLibraries.push_back(ubox<std::string>(line[2]));
            } else if (ubox<std::string>(line[1]) == "proc"){
                CLinkedProcedure linkproc(
                    {}, nullptr, ubox<std::string>(line[3])
                );
                if (line.size() == 5) continue;
                for (int i=4; i<line.size(); ++i){
                    auto content = ubox<std::string>(line[i]);
                    auto prevcontent = ubox<std::string>(line[i-1]);
                    if (content == "i64"){
                        if (prevcontent == "ret") {linkproc.ReturnType = llvm::Type::getInt64Ty(*GlobalContext); break;}
                        else linkproc.ArgTypes.push_back(llvm::Type::getInt64Ty(*GlobalContext));
                    } else if (content == "string"){
                        if (prevcontent == "ret") {linkproc.ReturnType = llvm::PointerType::getUnqual(*GlobalContext); break;}
                        else linkproc.ArgTypes.push_back(llvm::PointerType::getUnqual(*GlobalContext));
                    } else if (content == "void"){
                        if (prevcontent == "ret") {linkproc.ReturnType = llvm::Type::getVoidTy(*GlobalContext); break;}
                        else linkproc.ArgTypes.push_back(llvm::Type::getVoidTy(*GlobalContext));
                    }
                    else continue;
                }
                ExposedFunctions.push_back(linkproc);
            }
        }
        if (inproc) current_proc.content.push_back(line);
    }
    Log::DebugPrint(1, "Finished parsing!");
    return procedures;
}

int main(int argc, char* argv[]){
    llvm::cl::opt<std::string> InputFile(
        llvm::cl::Positional,
        llvm::cl::desc("<File name>"),
        llvm::cl::Required
    );

    llvm::cl::opt<int> Debug(
        "debug",
        llvm::cl::desc("<Debug level>"),
        llvm::cl::Optional
    );
    Debug.Default = 0;

    std::string OutputFile;

    llvm::cl::opt<std::string, true> OutputLong(
        "output",
        llvm::cl::desc("File to write to"),
        llvm::cl::location(OutputFile)
    );

    llvm::cl::opt<std::string, true> OutputShort(
        "o",
        llvm::cl::desc("File to write to"),
        llvm::cl::location(OutputFile)
    );

    llvm::cl::ParseCommandLineOptions(argc, argv);
    DebugLevel = Debug;

    std::ifstream File((std::string(OutputFile)));
    std::stringstream buffer;
    buffer << File.rdbuf();
    std::string file_txt = buffer.str();
    Log::Print("Compilation beginning for "+OutputFile+"...\n");
    llvm::LLVMContext ctx;
    GlobalContext = &ctx;
    auto module = std::make_unique<llvm::Module>("Ltran", ctx);
    llvm::IRBuilder<> builder(ctx);

    llvm::FunctionType* voidty = llvm::FunctionType::get(builder.getVoidTy(), false);
    llvm::FunctionType* i32ty = llvm::FunctionType::get(builder.getInt64Ty(), false);
    llvm::Type* StringTy = llvm::PointerType::getUnqual(ctx);
    Log::Print("Compiling builtins...\n");

    
    //
    // BUILT-INS vvv
    //

    std::map<std::string, std::function<llvm::Function*()>> builtins = {
        {"fetchli", [&](){
            llvm::Function *fetchli = llvm::Function::Create(
                llvm::FunctionType::get(
                    llvm::Type::getInt64Ty(ctx),
                    {StringTy, llvm::Type::getInt64Ty(ctx)},
                    false
                ),
                llvm::Function::ExternalLinkage,
                "fetchli",
                module.get()
            );

            llvm::BasicBlock* entry = llvm::BasicBlock::Create(ctx, "_ltran_fetchli", fetchli);
            llvm::IRBuilder<> local_builder(ctx);
            local_builder.SetInsertPoint(entry);
            auto args = fetchli->arg_begin();
            llvm::Value* temp_a = &*args++;
            llvm::Value* retval = &*args++;
            local_builder.CreateRet(retval);
            return fetchli;
        }},
        {"fetchls", [&](){
            llvm::Function *fetchls = llvm::Function::Create(
                llvm::FunctionType::get(
                    StringTy,
                    {StringTy, StringTy},
                    false
                ),
                llvm::Function::ExternalLinkage,
                "fetchls",
                module.get()
            );

            llvm::BasicBlock* fetchls_entry = llvm::BasicBlock::Create(ctx, "_ltran_fetchls", fetchls);
            llvm::IRBuilder<> local_builder(ctx);
            local_builder.SetInsertPoint(fetchls_entry);
            auto fetchls_args = fetchls->arg_begin();
            llvm::Value* fetchls_a = &*fetchls_args++;
            llvm::Value* fetchls_retval = &*fetchls_args++;
            local_builder.CreateRet(fetchls_retval);
            return fetchls;
        }},
        {"add", [&](){
            llvm::Function *add = llvm::Function::Create(
                llvm::FunctionType::get(
                    llvm::Type::getInt64Ty(ctx),
                    {llvm::Type::getInt64Ty(ctx), llvm::Type::getInt64Ty(ctx)},
                    false
                ),
                llvm::Function::ExternalLinkage,
                "add",
                module.get()
            );

            llvm::BasicBlock* entree = llvm::BasicBlock::Create(ctx, "_ltran_add", add);
            llvm::IRBuilder<> local_builder(ctx);
            local_builder.SetInsertPoint(entree);
            
            auto add_args = add->arg_begin();
            llvm::Value* evil_variable = &*add_args++;
            llvm::Value* pure_variable = &*add_args++;
            local_builder.CreateRet(local_builder.CreateAdd(evil_variable, pure_variable));
            return add;
        }},
        {"strptr", [&](){
            llvm::Function *strptr = llvm::Function::Create(
                llvm::FunctionType::get(
                    llvm::Type::getInt64Ty(ctx),
                    {StringTy},
                    false
                ),
                llvm::Function::ExternalLinkage,
                "strptr",
                module.get()
            );

            llvm::BasicBlock* entree = llvm::BasicBlock::Create(ctx, "_ltran_strptr", strptr);
            llvm::IRBuilder<> local_builder(ctx);
            local_builder.SetInsertPoint(entree);
            
            auto args = strptr->arg_begin();
            llvm::Value* evil_variable = &*args++;
            local_builder.CreateRet(local_builder.CreatePtrToInt(evil_variable, llvm::Type::getInt64Ty(*GlobalContext)));
            return strptr;
        }}
    };

    std::map<std::string, llvm::InlineAsm*> asm_builtins_registry = {};
    std::map<std::string, std::function<llvm::InlineAsm*()>> asm_builtins = {
        {"syscall", [&](){
            auto asmty = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(*GlobalContext),
            {
                llvm::Type::getInt64Ty(*GlobalContext),
                llvm::Type::getInt64Ty(*GlobalContext),
                llvm::Type::getInt64Ty(*GlobalContext),
                llvm::Type::getInt64Ty(*GlobalContext)
            },
            false
            );

            auto syscallfn = llvm::InlineAsm::get(
                asmty,
                "syscall",
                "={rax},{rax},{rdi},{rsi},{rdx}",
                true
            );
            asm_builtins_registry["syscall"] = syscallfn;
            return syscallfn;
        }}
    };
    
    auto gatedgetfn = [&](std::string fn_name) -> llvm::Function* {
        auto fn = module->getFunction(fn_name);
        if (module->getFunction(fn_name) == nullptr && !builtins.contains(fn_name)){
            return nullptr;
        } else if (builtins.contains(fn_name) && module->getFunction(fn_name) == nullptr){
            return builtins[fn_name]();
        } else {
            return fn;
        }
    };

    auto gatedgetfn_asm = [&](std::string fn_name) -> llvm::InlineAsm* {
        auto regiter = asm_builtins_registry.find(fn_name);
        if (regiter != asm_builtins_registry.end()) {
            return regiter->second;
        }

        if (builtins.contains(fn_name)) {
            return nullptr;
        }

        auto iter = asm_builtins.find(fn_name);
        if (iter != asm_builtins.end()) {
            return iter->second();
        }

        return nullptr;
    };
    //
    // BUILT-INS ^^^
    //
    auto parentheses_type_handler = [&](LtranObject Object){
        auto LtranObjectVector = ubox<std::vector<LtranObject>>(Object);
        auto fn_name = ubox<std::string>(LtranObjectVector[0]);
        auto func = gatedgetfn(fn_name);
        
        if (!func)
            return gatedgetfn_asm(fn_name)->getFunctionType()->getReturnType();
        return func->getReturnType();
    };

    auto llvmbuild = [&](this const auto& self, Procedure proc){
        for (auto Line : proc.content){
            if (Line[1].Signature == "call" && ubox<std::string>(Line[0]) == "ret"){
                proc.return_type = parentheses_type_handler(Line[1]);
            }
        }
        llvm::Function* proc_func = llvm::Function::Create(
            llvm::FunctionType::get(proc.return_type, proc.arg_types, false),
            llvm::Function::ExternalLinkage,
            proc.name,
            module.get()
        );
        
        Log::DebugPrint(1, "Made function for '"+proc.name+"'...\n");

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(
            ctx,
            "_ltran_"+proc.name,
            proc_func
        );

        builder.SetInsertPoint(entry);
        for (auto Line : proc.content){

            std::vector<llvm::Value*> llvmvect = {};
            // assume first is label for now. later, throw error if such is not true.
            auto finish_call = [&proc_func, &builder, &module, &llvmvect, &proc, &gatedgetfn_asm, &gatedgetfn](this const auto& self, std::vector<LtranObject> objs) -> llvm::Value* {
                auto name = ubox<std::string>(objs[0]);
                
                auto asm_func = gatedgetfn_asm(name);
                auto func = gatedgetfn(name);
                std::vector<llvm::Value*> arguments = {};
                for (auto Object : objs) {
                    if (Object.Signature == "integer"){
                        arguments.push_back(builder.getInt64((uint64_t)ubox<int>(Object)));
                    } else if (Object.Signature == "string"){
                        arguments.push_back(builder.CreateGlobalStringPtr(ubox<std::string>(Object)));
                    } else if (Object.Signature == "call"){
                        arguments.push_back(self(ubox<std::vector<LtranObject>>(Object)));
                    }
                }
                if (!asm_func){
                    auto iter = proc_func->arg_begin();
                    if (name == "fetchli" || name == "fetchls"){
                        // assume arg is string
                        auto arg = ubox<std::string>(objs[1]);
                        for (auto argument : proc.Args){
                            auto argname = ubox<std::string>(argument);
                            if (argname == arg){
                                arguments.push_back(&*iter);
                                break;
                            }
                            ++iter;
                        }
                    }
                    return builder.CreateCall(func, arguments);
                } else {
                    return builder.CreateCall(asm_func, arguments);
                }
            };
            if (ubox<std::string>(Line[0]) == "ret"){
                if (Line[1].Signature == "call"){
                    builder.CreateRet(
                        finish_call(ubox<std::vector<LtranObject>>(Line[1]))
                    );
                }
                else if (proc.return_type == llvm::PointerType::getUnqual(ctx)){
                    builder.CreateRet(
                        builder.CreateGlobalStringPtr(ubox<std::string>(Line[1]))
                    );
                }
                else if (proc.return_type == llvm::Type::getInt64Ty(ctx)){
                    builder.CreateRet(
                        builder.getInt64((uint64_t)ubox<int>(Line[1]))
                    );
                }
                continue;
            }
            llvm::Function *func = gatedgetfn(ubox<std::string>(Line[0]));
            llvm::InlineAsm *asm_func = gatedgetfn_asm(ubox<std::string>(Line[0]));
            Log::DebugPrint(2, "Got function for ln of '"+proc.name+"'...\n");

            if (func == nullptr && asm_func == nullptr){
                Log::Error(true, "No such function as '"+ubox<std::string>(Line[0])+"'. Compilation terminated.\n");
            }
            // Zero Arg Case Segfault Must Die -ii-
            if (Line.size() != 1){
                if (DEBUG)
                for (int i=1; i<Line.size(); ++i){
                    if (Line[i].Signature == "integer") {
                        llvmvect.push_back(
                            builder.getInt64(ubox<int>(Line[i]))
                        );
                    }
                    else if (Line[i].Signature == "string") {
                        llvmvect.push_back(
                            builder.CreateGlobalStringPtr(ubox<std::string>(Line[i]))
                        );
                    }
                    else if (Line[i].Signature == "call") {
                        // finish call
                        llvmvect.push_back(finish_call(ubox<std::vector<LtranObject>>(Line[i])));
                    }
                }
            }
            if (func != nullptr){
                builder.CreateCall(func, llvmvect);
            } else {
                builder.CreateCall(asm_func, llvmvect);
            }
    }};
    Log::Print("Linking modules...");
    if (!ExposedFunctions.empty()){
        for (CLinkedProcedure linkproc : ExposedFunctions){
            llvm::Function::Create(
                llvm::FunctionType::get(
                    linkproc.ReturnType,
                    linkproc.ArgTypes,
                    false
                ),
                llvm::Function::ExternalLinkage,
                linkproc.Name,
                module.get()
            );
        }
    }

    if (!IncludedFiles.empty()){
        for (auto File : IncludedFiles){
            std::ifstream included_file((std::string(InputFile)));
            std::stringstream buff;
            buff << included_file.rdbuf();
            std::string txt = buff.str();

            Log::Print("Parsing "+File+"...\n");
            auto procs = Parser(txt);
            for (auto proc : procs){
                llvmbuild(proc);
            }
        }
    }
    Log::Print("Parsing "+InputFile+"...\n");

    auto procedures = Parser(file_txt);
    for (auto proc : procedures){
        Log::Print("Building procedure "+proc.name+"...\n");
        llvmbuild(proc);
    }

    Log::Print("Verifying LLVM IR...\n");
    if (!llvm::verifyModule(*module, &llvm::errs())){
        // output to file for now
        std::error_code ec;
        llvm::raw_fd_ostream out("output.ll", ec);

        module->print(out, nullptr);
        system("clang output.ll -o output -nodefaultlibs");
        if (DebugLevel < 1) system("rm output.ll");
        Log::Print("All done with compilation!\n");
    } else {
        Log::Error(true, "Problem with LLVM IR. If you are a regular user seeing this, please report it on GitHub. Sorry :<\n", -5);
    }
}
