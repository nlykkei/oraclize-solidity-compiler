/** 
 *  @file    OraclizePass.cpp
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Oraclize compiler pass.
 *
 *  @section DESCRIPTION
 *  
 *  Compiler pass for oraclizing contracts.
 */

#include <libsolidity/ast/AST.h>
#include <libsolidity/interface/ErrorReporter.h>
#include <libsolidity/ast/ASTPrinter.h>

#include <libsolidity/oraclize/OraclizePass.h>
#include <libsolidity/oraclize/OraclizeTransform.h>
#include <libsolidity/oraclize/OraclizeCommon.h>

#include <memory>
#include <iostream>
#include <sstream>

using namespace std;
using namespace dev;
using namespace dev::solidity;

bool OraclizePass::analyze(const SourceUnit &source_unit)
{
    oraclize_transform_ = make_shared<OraclizeTransform>(&source_unit, gas_limit_, gas_price_);

    source_unit.accept(*this);

    return Error::containsOnlyWarnings(error_reporter_.errors());
}

bool OraclizePass::visit(const ContractDefinition &contract)
{
    vector<string> ignore{"OraclizeI", "OraclizeAddrResolverI", "usingOraclize",
                          "strings", "OraclizeSolidity"};

#if defined(COMPILER_DEBUG)

    cerr << "[Debug] visit(ContractDefinition): " << contract.name() << endl;

#endif

    if (find(ignore.begin(), ignore.end(), contract.name()) != ignore.end())
    {

#if defined(COMPILER_DEBUG)

        cerr << "[Ignore] " << contract.name() << endl;

#endif

        return false; // Ignore Oraclize contracts
    }

    return true;
}

void OraclizePass::endVisit(const ContractDefinition &contract)
{

#if defined(COMPILER_DEBUG)

    cerr << "[Debug] endvisit(ContractDefinition): " << contract.name() << endl;

#endif

    (void)contract;

    if (oracle_queries_.empty())
    {
        return; // Contract contains no oracle calls
    }

    oraclize_transform_->OraclizeEnvironment(contract, oracle_queries_);
    oraclize_transform_->OraclizeEvent(contract);
    oraclize_transform_->OraclizeContainer(oracle_queries_);
    oraclize_transform_->OraclizeCallback(contract, oracle_queries_);

#if defined(COMPILER_DEBUG)

    cerr << "[Debug] Oracle queries:" << endl
         << endl;

    for (auto &oracle_query : oracle_queries_)
    {
        cerr << oracle_query->ToString() << endl;
    }

#endif

#if defined(AST_DEBUG)

    ASTPrinter ast_printer(contract);
    ast_printer.print(cerr);

#endif

    oracle_queries_.clear(); // Clear list of oracle calls
}

bool OraclizePass::visit(const FunctionDefinition &function)
{

#if defined(COMPILER_DEBUG)

    cerr << "[Debug] visit(FunctionDefinition): " << function.name() << endl;

#endif

    current_func_ = &function;

    return true;
}

void OraclizePass::endVisit(const FunctionDefinition &function)
{

#if defined(COMPILER_DEBUG)

    cerr << "[Debug] endvisit(FunctionDefinition): " << function.name() << endl;

#endif

    (void)function;

    current_func_ = nullptr;
}

bool OraclizePass::visit(const Identifier &identifier)
{

#if defined(COMPILER_DEBUG)

    cerr << "[Debug] visit(Identifier): " << identifier.name() << endl;

#endif

    (void)identifier;

    return true;
}

bool OraclizePass::visit(const VariableDeclaration &variable)
{

#if defined(COMPILER_DEBUG)

    cerr << "[Debug] visit(VariableDeclaration): " << variable.name() << endl;

#endif

    (void)variable;

    return true;
}

bool OraclizePass::visit(const FunctionCall &call)
{

#if defined(COMPILER_DEBUG)

    auto debug_identifier = dynamic_cast<const Identifier *>(&call.expression());

    if (debug_identifier)
    {
        cerr << "[Debug] visit(FunctionCall): " << debug_identifier->name() << endl;
    }
    else
    {
        cerr << "[Debug] visit(FunctionCall)" << endl;
    }

#endif

    Expression const &call_expr = call.expression();

    const Identifier *call_identifier = dynamic_cast<const Identifier *>(&call_expr);

    if (call_identifier) // Check if call-expression is an identifier (first-order call)
    {
        // Check identifier for "oracleQuery"
        if (GetOracleIdentifier().compare(call_identifier->name()) != 0 || current_func_ == nullptr)
        {
            return true; // Not proper oracle call: Continue
        }

        auto call_args = call.arguments();

        // Found oracle call: oracleQuery(...)

        // Check for at least two arguments: type, ..., callback
        if (call_args.size() < 2)
        {
            return true;
        }

        // Check oracle type (string), e.g., "data"
        const Literal *oracle_type_literal = dynamic_cast<const Literal *>(call_args[0].get());

        if (!oracle_type_literal || oracle_type_literal->token() != Token::StringLiteral)
        {
            return true;
        }

        auto it = stringToOracleType.find(oracle_type_literal->value());

        if (it == stringToOracleType.end())
        {
            return true;
        }

        OracleType oracle_type = it->second;

        switch (oracle_type)
        {
        case OracleType::Data:
        {
            // Arguments: type, URL..., callback
            if (call_args.size() < 3)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Missing arguments for \"data\" query.");
            }

            vector<string> urls;

            for (uint i = 1; i < call_args.size() - 1; ++i)
            {
                urls.push_back(ParseString(call_args, i));
            }

            oracle_queries_.push_back(make_shared<DataQuery>(oracle_type,
                                                             urls,
                                                             ParseIdentifier(call_args, call_args.size() - 1),
                                                             const_cast<FunctionDefinition *>(current_func_)));
            break;
        }
        case OracleType::Min:
        {
            // Arguments: type, identifier, callback, [URL], [switch]
            if (call_args.size() < 3)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Missing arguments for \"min\" query.");
            }

            if (call_args.size() > 5)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too many arguments for \"min\" query.");
            }

            oracle_queries_.push_back(make_shared<MinQuery>(oracle_type,
                                                            ParseIdentifier(call_args, 1),
                                                            ParseIdentifier(call_args, 2),
                                                            call_args.size() >= 4 ? ParseString(call_args, 3) : "",
                                                            call_args.size() >= 5 ? ParseIdentifier(call_args, 4) : ASTPointer<Identifier>(),
                                                            const_cast<FunctionDefinition *>(current_func_)));
            break;
        }
        case OracleType::Sort:
        {
            // Arguments: type, identifier, callback, [verify], [URL], [switch]
            if (call_args.size() < 3)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too few arguments for \"sort\" query.");
            }

            if (call_args.size() > 6)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too many arguments for \"sort\" query.");
            }

            oracle_queries_.push_back(make_shared<SortQuery>(oracle_type,
                                                             ParseIdentifier(call_args, 1),
                                                             ParseIdentifier(call_args, 2),
                                                             call_args.size() >= 4 ? ParseBool(call_args, 3) : false,
                                                             call_args.size() >= 5 ? ParseString(call_args, 4) : "",
                                                             call_args.size() >= 6 ? ParseIdentifier(call_args, 5) : ASTPointer<Identifier>(),
                                                             const_cast<FunctionDefinition *>(current_func_)));
            break;
        }
        case OracleType::Sqrt:
        {
            // Arguments: type, (identifier | literal), callback, [verify], [URL], [switch]
            if (call_args.size() < 3)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too few arguments for \"sqrt\" query.");
            }

            if (call_args.size() > 6)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too many arguments for \"sqrt\" query.");
            }

            oracle_queries_.push_back(make_shared<SqrtQuery>(oracle_type,
                                                             ParseIdentifierOrNumber(call_args, 1),
                                                             ParseIdentifier(call_args, 2),
                                                             call_args.size() >= 4 ? ParseBool(call_args, 3) : false,
                                                             call_args.size() >= 5 ? ParseString(call_args, 4) : "",
                                                             call_args.size() >= 6 ? ParseIdentifier(call_args, 5) : ASTPointer<Identifier>(),
                                                             const_cast<FunctionDefinition *>(current_func_)));
            break;
        }
        case OracleType::ThreeSum:
        {
            // Arguments: type, identifier, (identifier | literal), callback, [verify], [URL], [switch]
            if (call_args.size() < 4)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too few arguments for \"threeSum\" query.");
            }

            if (call_args.size() > 7)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too many arguments for \"threeSum\" query.");
            }

            oracle_queries_.push_back(make_shared<ThreeSumQuery>(oracle_type,
                                                                 ParseIdentifier(call_args, 1),
                                                                 ParseIdentifierOrNumber(call_args, 2),
                                                                 ParseIdentifier(call_args, 3),
                                                                 call_args.size() >= 5 ? ParseBool(call_args, 4) : false,
                                                                 call_args.size() >= 6 ? ParseString(call_args, 5) : "",
                                                                 call_args.size() >= 7 ? ParseIdentifier(call_args, 6) : ASTPointer<Identifier>(),
                                                                 const_cast<FunctionDefinition *>(current_func_)));
            break;
        }
        case OracleType::KP:
        {
            // Arguments: type, identifier, (literal | identifier), (literal | identifier), callback, [verify], [URL], [switch]
            if (call_args.size() < 5)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too few arguments for \"kp\" query.");
            }

            if (call_args.size() > 8)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too many arguments for \"kp\" query.");
            }

            oracle_queries_.push_back(make_shared<KPQuery>(oracle_type,
                                                           ParseIdentifier(call_args, 1),
                                                           ParseIdentifierOrNumber(call_args, 2),
                                                           ParseIdentifierOrNumber(call_args, 3),
                                                           ParseIdentifier(call_args, 4),
                                                           call_args.size() >= 6 ? ParseBool(call_args, 5) : false,
                                                           call_args.size() >= 7 ? ParseString(call_args, 6) : "",
                                                           call_args.size() >= 8 ? ParseIdentifier(call_args, 7) : ASTPointer<Identifier>(),
                                                           const_cast<FunctionDefinition *>(current_func_)));
            break;
        }
        case OracleType::APSP:
        {
            // Arguments: type, identifier, callback, [URL], [switch]
            if (call_args.size() < 3)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too few arguments for \"apsp\" query.");
            }

            if (call_args.size() > 5)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too many arguments for \"apsp\" query.");
            }

            oracle_queries_.push_back(make_shared<APSPQuery>(oracle_type,
                                                             ParseIdentifier(call_args, 1),
                                                             ParseIdentifier(call_args, 2),
                                                             call_args.size() >= 4 ? ParseString(call_args, 3) : "",
                                                             call_args.size() >= 5 ? ParseIdentifier(call_args, 4) : ASTPointer<Identifier>(),
                                                             const_cast<FunctionDefinition *>(current_func_)));
            break;
        }
        case OracleType::KDS:
        {
            // Arguments: type, identifier, (literal | identifier), callback, [verify], [URL], [switch]
            if (call_args.size() < 4)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too few arguments for \"kds\" query.");
            }

            if (call_args.size() > 7)
            {
                throw OraclizeSolidityException("OraclizePass::visit: Too many arguments for \"kds\" query.");
            }

            oracle_queries_.push_back(make_shared<KDSQuery>(oracle_type,
                                                            ParseIdentifier(call_args, 1),
                                                            ParseIdentifierOrNumber(call_args, 2),
                                                            ParseIdentifier(call_args, 3),
                                                            call_args.size() >= 5 ? ParseBool(call_args, 4) : false,
                                                            call_args.size() >= 6 ? ParseString(call_args, 5) : "",
                                                            call_args.size() >= 7 ? ParseIdentifier(call_args, 6) : ASTPointer<Identifier>(),
                                                            const_cast<FunctionDefinition *>(current_func_)));
            break;
        }
        default:
        {
            throw OraclizeSolidityException("OraclizePass::visit: Unknown query type.");
        }
        }
    }

    return true;
}

ASTPointer<Identifier> OraclizePass::ParseIdentifier(const vector<ASTPointer<const Expression>> &call_args, const size_t pos)
{
    const Identifier *identifier = dynamic_cast<const Identifier *>(call_args[pos].get());

    if (!identifier)
    {
        throw OraclizeSolidityException("OraclizePass::ParseIdentifier: No identifier at position.");
    }

    // Type checking on identifier by means of called function
    return ASTPointer<Identifier>(const_pointer_cast<Identifier>(dynamic_pointer_cast<const Identifier>(call_args[pos])));
}

ASTPointer<Expression> OraclizePass::ParseIdentifierOrNumber(const vector<ASTPointer<const Expression>> &call_args, const size_t pos)
{
    const Identifier *identifier = dynamic_cast<const Identifier *>(call_args[pos].get());

    if (identifier)
    {
        return const_pointer_cast<Identifier>(dynamic_pointer_cast<const Identifier>(call_args[pos]));
    }

    const Literal *literal = dynamic_cast<const Literal *>(call_args[pos].get());

    if (literal && literal->token() == Token::Number)
    {
        return const_pointer_cast<Literal>(dynamic_pointer_cast<const Literal>(call_args[pos]));
    }

    throw OraclizeSolidityException("OraclizePass::ParseIdentifierOrNumber: No identifier or number at position.");
}

bool OraclizePass::ParseBool(const vector<ASTPointer<const Expression>> &call_args, const size_t pos)
{
    const Literal *verify_literal = dynamic_cast<const Literal *>(call_args[pos].get());

    if (!verify_literal)
    {
        throw OraclizeSolidityException("OraclizePass::ParseBool: No bool at position.");
    }

    switch (verify_literal->token())
    {
    case Token::TrueLiteral:
    {
        return true;
    }
    case Token::FalseLiteral:
    {
        return false;
    }
    default:
    {
        throw OraclizeSolidityException("OraclizePass::ParseBool: No bool at position.");
    }
    }
}

string OraclizePass::ParseString(const vector<ASTPointer<const Expression>> &call_args, const size_t pos)
{
    const Literal *literal = dynamic_cast<const Literal *>(call_args[pos].get());

    if (literal && literal->token() == Token::StringLiteral)
    {
        return literal->value();
    }
    else
    {
        throw OraclizeSolidityException("OraclizePass::ParseString: No string at position.");
    }
}