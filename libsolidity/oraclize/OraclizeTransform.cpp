/** 
 *  @file    OraclizeTransform.cpp
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Oraclize transformation.
 *
 *  @section DESCRIPTION
 *  
 *  Transformation for oraclizing contracts.
 */

#include <sstream>
#include <string>

#include <libsolidity/oraclize/OraclizeTransform.h>

using namespace std;
using namespace dev;
using namespace dev::solidity;

bool OraclizeTransform::OraclizeEnvironment(const ContractDefinition &contract, const vector<shared_ptr<OracleQuery>> &oracle_queries)
{
    auto &sub_nodes = contract.subNodesByRef();
    uint index = 0;

    for (auto &oracle_query : oracle_queries)
    {
        // Environment definition
        oracle_query->set_env_name(string("OEnv") + to_string(index));

        sub_nodes.insert(sub_nodes.begin() + index, EnvironmentDef(make_shared<ASTString>(oracle_query->env_name()), oracle_query));

        // Environment variable
        string var_name = oracle_query->env_name();
        var_name[0] = tolower(var_name[0]);
        var_name = string("_") + var_name;
        oracle_query->set_var_name(var_name);

        auto type = make_shared<UserDefinedTypeName>(empty_loc_, vector<ASTString>{oracle_query->env_name()});
        auto var_decl = make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>(oracle_query->var_name()),
                                                         ASTPointer<Expression>(), Declaration::Visibility::Private, true);

        sub_nodes.insert(sub_nodes.begin() + 2 * index + 1, var_decl);

        index += 1;
    }

    return true;
}

bool OraclizeTransform::OraclizeEvent(const ContractDefinition &contract)
{
    auto &sub_nodes = contract.subNodesByRef();

    auto var_decl0 = make_shared<VariableDeclaration>(empty_loc_,
                                                      make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken(Token::BytesM, 32, 0)),
                                                      make_shared<ASTString>("queryID"),
                                                      ASTPointer<Expression>(),
                                                      Declaration::Visibility::Default);

    auto var_decl1 = make_shared<VariableDeclaration>(empty_loc_,
                                                      make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken(Token::String, 0, 0)),
                                                      make_shared<ASTString>("type"),
                                                      ASTPointer<Expression>(),
                                                      Declaration::Visibility::Default);

    auto var_decl2 = make_shared<VariableDeclaration>(empty_loc_,
                                                      make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken(Token::String, 0, 0)),
                                                      make_shared<ASTString>("what"),
                                                      ASTPointer<Expression>(),
                                                      Declaration::Visibility::Default);

    auto event_params = make_shared<ParameterList>(empty_loc_, vector<ASTPointer<VariableDeclaration>>{var_decl0, var_decl1, var_decl2});

    auto oraclize_event = make_shared<EventDefinition>(empty_loc_,
                                                       make_shared<ASTString>("OraclizeEvent"),
                                                       empty_doc_,
                                                       event_params);

    sub_nodes.insert(sub_nodes.begin(), oraclize_event);

    return true;
}

bool OraclizeTransform::OraclizeContainer(const vector<shared_ptr<OracleQuery>> &oracle_queries)
{
    /* 
     * Container:
     * 
     * ...
     * _oEnvN = OEnvN(...);
     * _oEnvN.queryId = oraclize_query("URL", url);
     * OraclizeEvent(_OEnvN.queryId, queryType, func_name);
     * ...
     */

    for (auto &oracle_query : oracle_queries)
    {
        vector<ASTPointer<Statement>> &func_stmts = oracle_query->container_func()->body().statementsRef();
        vector<ASTPointer<Statement>>::iterator it = func_stmts.begin();

        const ExpressionStatement *expr_stmt;
        const FunctionCall *func_call;
        const Identifier *func_id;

        // Find and delete oracle call
        while (it != func_stmts.end())
        {
            if ((expr_stmt = dynamic_cast<const ExpressionStatement *>(it->get())))
            {
                if ((func_call = dynamic_cast<const FunctionCall *>(&expr_stmt->expression())))
                {
                    if ((func_id = dynamic_cast<const Identifier *>(&func_call->expression())))
                    {
                        if (GetOracleIdentifier().compare(func_id->name()) == 0)
                        {
                            it = func_stmts.erase(it);
                            break;
                        }
                    }
                }
            }

            it++;
        }

        // Environment init (call position): _oEnvN = OEnvN(...);
        it = func_stmts.insert(it, make_shared<ExpressionStatement>(empty_loc_, empty_doc_, EnvironmentInit(oracle_query)));

        if (gas_price_)
        {
            // User-specified gas price (default: 20 GWei)
            auto gas_price = make_shared<Literal>(empty_loc_,
                                                  Token::Number,
                                                  make_shared<ASTString>(to_string(gas_price_)));

            // oraclize_setCustomGasPrice(gas_price);
            it = func_stmts.insert(it + 1, make_shared<ExpressionStatement>(empty_loc_,
                                                                            empty_doc_,
                                                                            make_shared<FunctionCall>(empty_loc_,
                                                                                                      make_shared<Identifier>(empty_loc_,
                                                                                                                              make_shared<ASTString>("oraclize_setCustomGasPrice")),
                                                                                                      vector<ASTPointer<Expression>>{gas_price},
                                                                                                      vector<ASTPointer<ASTString>>{})));
        }

        // _oEnvN.queryId = oraclize_query("URL", url);
        auto oraclize = OraclizeQuery(oracle_query);

        vector<ASTPointer<Statement>> stmts;

        auto assign = make_shared<Assignment>(empty_loc_,
                                              make_shared<MemberAccess>(empty_loc_,
                                                                        make_shared<Identifier>(empty_loc_,
                                                                                                make_shared<ASTString>(oracle_query->var_name())),
                                                                        make_shared<ASTString>("queryId")),
                                              Token::Assign,
                                              oraclize.first);

        stmts.push_back(make_shared<ExpressionStatement>(empty_loc_, empty_doc_, assign));

#if defined(CONTRACT_DEBUG)

        // OraclizeEvent(_OEnvN.queryId, queryType, url);
        auto log_url = make_shared<ExpressionStatement>(empty_loc_,
                                                        empty_doc_,
                                                        make_shared<FunctionCall>(empty_loc_,
                                                                                  make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                  vector<ASTPointer<Expression>>{
                                                                                      make_shared<MemberAccess>(empty_loc_,
                                                                                                                make_shared<Identifier>(empty_loc_,
                                                                                                                                        make_shared<ASTString>(oracle_query->var_name())),
                                                                                                                make_shared<ASTString>("queryId")),
                                                                                      make_shared<Literal>(empty_loc_,
                                                                                                           Token::StringLiteral,
                                                                                                           make_shared<ASTString>(oracleTypeToString.at(oracle_query->type()))),
                                                                                      oraclize.second},
                                                                                  vector<ASTPointer<ASTString>>{}));

        stmts.push_back(log_url);

#endif

        // Dynamic switching
        it = func_stmts.insert(it + 1, DynamicSwitching(make_shared<Block>(empty_loc_, empty_doc_, stmts), oracle_query));

#if defined(CONTRACT_DEBUG)

        // OraclizeEvent(_OEnvN.queryId, queryType, func);
        auto log_container = make_shared<ExpressionStatement>(empty_loc_,
                                                              empty_doc_,
                                                              make_shared<FunctionCall>(empty_loc_,
                                                                                        make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                        vector<ASTPointer<Expression>>{
                                                                                            make_shared<MemberAccess>(empty_loc_,
                                                                                                                      make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                                                      make_shared<ASTString>("queryId")),
                                                                                            make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(oracleTypeToString.at(oracle_query->type()))),
                                                                                            make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(oracle_query->container_func()->name()))},
                                                                                        vector<ASTPointer<ASTString>>{}));

        it = func_stmts.insert(it + 1, log_container);

#endif
    }

    return true;
}

bool OraclizeTransform::OraclizeCallback(const ContractDefinition &contract, const vector<shared_ptr<OracleQuery>> &oracle_queries)
{
    /* 
     * Callback:
     * 
     * __callback(bytes32 _queryId, string _result) {
     *    OraclizeEvent(_queryId, "", "__callback() received:");
     *    OraclizeEvent(_queryId, "", _result);
     *
     *    if (_queryId == _oEnv0.queryId)
     *    {
     *       ...
     *    }
     *    else if (_queryId == _oEnv1.queryId)
     *    {
     *       ...
     *    }
     *    ...
     *    else if (_queryId == _oEnvN.queryId)
     *    {
     *       ...
     *    }
     *    else
     *    {
     *       ...
     *    }
     * }
     */

    auto &sub_nodes = contract.subNodesByRef();

    vector<shared_ptr<OracleQuery>> oracle_queries_rev(oracle_queries);
    reverse(oracle_queries_rev.begin(), oracle_queries_rev.end());

    ASTPointer<IfStatement> current_if_stmt;
    bool first_iteration = true;
    uint i = 0;

    for (auto &oracle_query : oracle_queries_rev)
    {
        ASTPointer<Expression> lhs;
        ASTPointer<Expression> rhs;
        vector<ASTPointer<Statement>> stmts;
        vector<ASTPointer<Expression>> call_args;

        switch (oracle_query->type())
        {
        case OracleType::Data:
        {
            const auto query_size = oracle_query->QuerySize();

            if (query_size == 1)
            {
                /*
                 * if (_queryId == _oEnvN.queryId) {
                 *    callback(_result);
                 *    delete _oEnvN;
                 * }
                 */

                // callback(_result);
                call_args.push_back(make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result")));

                stmts.push_back(make_shared<ExpressionStatement>(empty_loc_,
                                                                 empty_doc_,
                                                                 make_shared<FunctionCall>(empty_loc_,
                                                                                           oracle_query->callback(),
                                                                                           call_args,
                                                                                           vector<ASTPointer<ASTString>>{})));

                // delete _oEnvN;
                stmts.push_back(make_shared<ExpressionStatement>(empty_loc_,
                                                                 empty_doc_,
                                                                 make_shared<UnaryOperation>(empty_loc_,
                                                                                             Token::Delete,
                                                                                             make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                             true)));
            }
            else
            {
                /*
                 * if (_queryId == _oEnvN.queryId) {
                 *    _oEnvN.retVals[_oEnvN.index] = _result;
                 *    _oEnvN.index += 1;
                 *    if (_oEnvN.index == nURL) {
                 *       callback(_oEnvN.retVals[0], ..., _oEnvN.retVals[nURL]);
                 *       delete _oEnvN;
                 *    } 
                 *    else {
                 *       _oEnvN.queryId = oraclize_query("URL", _oEnvN.urls[_oEnvN.index]);
                 *       OraclizeEvent(_oEnvN.queryId, queryType, _oEnvN.urls[_oEnvN.index]);
                 *    }
                 * }
                 */

                // _oEnvN.retVals[_oEnvN.index] = _result;
                lhs = make_shared<IndexAccess>(empty_loc_,
                                               make_shared<MemberAccess>(empty_loc_,
                                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                         make_shared<ASTString>("retVals")),
                                               make_shared<MemberAccess>(empty_loc_,
                                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                         make_shared<ASTString>("index")));

                rhs = make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result"));

                stmts.push_back(make_shared<ExpressionStatement>(empty_loc_,
                                                                 empty_doc_,
                                                                 make_shared<Assignment>(empty_loc_, lhs, Token::Assign, rhs)));

                // _oEnvN.index += 1;
                lhs = make_shared<MemberAccess>(empty_loc_,
                                                make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                make_shared<ASTString>("index"));

                rhs = make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("1"));

                stmts.push_back(make_shared<ExpressionStatement>(empty_loc_,
                                                                 empty_doc_,
                                                                 make_shared<Assignment>(empty_loc_, lhs, Token::AssignAdd, rhs)));

                // _oEnvN.index == nURL
                auto cond = make_shared<BinaryOperation>(empty_loc_,
                                                         make_shared<MemberAccess>(empty_loc_,
                                                                                   make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                   make_shared<ASTString>("index")),
                                                         Token::Equal,
                                                         make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>(to_string(query_size))));

                call_args.clear();

                // callback(_oEnvN.retVals[0], ..., _oEnvN.retVals[nURL]);
                for (uint i = 0; i < query_size; ++i)
                {
                    call_args.push_back(make_shared<IndexAccess>(empty_loc_,
                                                                 make_shared<MemberAccess>(empty_loc_,
                                                                                           make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                           make_shared<ASTString>("retVals")),
                                                                 make_shared<Literal>(empty_loc_,
                                                                                      Token::Number,
                                                                                      make_shared<ASTString>(to_string(i)))));
                }

                auto callback = make_shared<ExpressionStatement>(empty_loc_,
                                                                 empty_doc_,
                                                                 make_shared<FunctionCall>(empty_loc_,
                                                                                           oracle_query->callback(),
                                                                                           call_args,
                                                                                           vector<ASTPointer<ASTString>>{}));
                // delete _oEnvN;
                auto delete_env = make_shared<ExpressionStatement>(empty_loc_,
                                                                   empty_doc_,
                                                                   make_shared<UnaryOperation>(empty_loc_,
                                                                                               Token::Delete,
                                                                                               make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                               true));

                auto true_branch = make_shared<Block>(empty_loc_,
                                                      empty_doc_,
                                                      vector<ASTPointer<Statement>>{callback, delete_env});

                // _oEnvN.queryId = oraclize_query("URL", _oEnvN.urls[_oEnvN.index]);
                auto oraclize_query = OraclizeQuery(oracle_query);

                auto assign = make_shared<ExpressionStatement>(empty_loc_,
                                                               empty_doc_,
                                                               make_shared<Assignment>(empty_loc_,
                                                                                       make_shared<MemberAccess>(empty_loc_,
                                                                                                                 make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                                                 make_shared<ASTString>("queryId")),
                                                                                       Token::Assign,
                                                                                       oraclize_query.first));

                vector<ASTPointer<Statement>> false_stmts;
                false_stmts.push_back(assign);

#if defined(CONTRACT_DEBUG)

                // OraclizeEvent(_oEnvN.queryId, queryType, _oEnvN.urls[_oEnvN.index]);
                auto log_url = make_shared<ExpressionStatement>(empty_loc_,
                                                                empty_doc_,
                                                                make_shared<FunctionCall>(empty_loc_,
                                                                                          make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                          vector<ASTPointer<Expression>>{
                                                                                              make_shared<MemberAccess>(empty_loc_,
                                                                                                                        make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                                                        make_shared<ASTString>("queryId")),
                                                                                              make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(oracleTypeToString.at(oracle_query->type()))),
                                                                                              oraclize_query.second},
                                                                                          vector<ASTPointer<ASTString>>{}));

                false_stmts.push_back(log_url);

#endif

                auto false_branch = make_shared<Block>(empty_loc_,
                                                       empty_doc_,
                                                       false_stmts);

                /*    
                 * if (_oEnvN.index == nURL) {
                 *    ...
                 * }
                 * else {
                 *    ...
                 * }
                 */

                stmts.push_back(make_shared<IfStatement>(empty_loc_, empty_doc_, cond, true_branch, false_branch));
            }
            break;
        }
        case OracleType::Sort:
        case OracleType::Min:
        case OracleType::Sqrt:
        case OracleType::ThreeSum:
        case OracleType::KP:
        case OracleType::APSP:
        case OracleType::KDS:
        {
            /*
             * if (_queryId == _oEnvN.queryId) {
             *    [Verification]
             *    callback(_result);
             *    delete _oEnvN;
             * }
             */

            // [Verification]
            auto query = dynamic_cast<VerifierQuery *>(oracle_query.get());
            if (query && query->verify())
            {
                switch (oracle_query->type())
                {
                case OracleType::Sqrt:
                {
                    /*
                     * uint _sqrt = parseInt(_result);
                     * if (_sqrt**2 <= _oEnvN.sqrt && (_sqrt + 1)**2 > _oEnvN.sqrt) {
                     *    OraclizeEvent(_queryId, "sqrt", "Valid result");
                     * }
                     * else {
                     *    OraclizeEvent(_queryId, "sqrt", "Invalid result");
                     *    _result = "";
                     * }
                     */

                    call_args.clear();

                    // uint _sqrt = parseInt(_result);
                    call_args.push_back(make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result")));

                    auto init_val = make_shared<FunctionCall>(empty_loc_,
                                                              make_shared<Identifier>(empty_loc_, make_shared<ASTString>("parseInt")),
                                                              call_args,
                                                              vector<ASTPointer<ASTString>>{});

                    auto var_decl = make_shared<VariableDeclaration>(empty_loc_,
                                                                     make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0}),
                                                                     make_shared<ASTString>(string("_sqrt") + to_string(i)),
                                                                     ASTPointer<Expression>(),
                                                                     Declaration::Visibility::Default);

                    stmts.push_back(make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                              empty_doc_,
                                                                              vector<ASTPointer<VariableDeclaration>>{var_decl},
                                                                              init_val));

                    // _sqrt**2 <= _oEnvN.sqrt
                    auto and0 = make_shared<BinaryOperation>(empty_loc_,
                                                             make_shared<BinaryOperation>(empty_loc_,
                                                                                          make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_sqrt") + to_string(i))),
                                                                                          Token::Exp,
                                                                                          make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("2"))),
                                                             Token::LessThanOrEqual,
                                                             make_shared<MemberAccess>(empty_loc_,
                                                                                       make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                       make_shared<ASTString>("sqrt")));

                    // (_sqrt + 1)**2 > _oEnvN.sqrt
                    auto and1 = make_shared<BinaryOperation>(empty_loc_,
                                                             make_shared<BinaryOperation>(empty_loc_,
                                                                                          make_shared<BinaryOperation>(empty_loc_,
                                                                                                                       make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_sqrt") + to_string(i))),
                                                                                                                       Token::Add,
                                                                                                                       make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("1"))),

                                                                                          Token::Exp,
                                                                                          make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("2"))),
                                                             Token::GreaterThan,
                                                             make_shared<MemberAccess>(empty_loc_,
                                                                                       make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                       make_shared<ASTString>("sqrt")));

                    // (_sqrt**2 <= _oEnvN.sqrt) && ((_sqrt + 1)**2 > _oEnvN.sqrt)
                    auto and_cond = make_shared<BinaryOperation>(empty_loc_, and0, Token::And, and1);

                    vector<ASTPointer<Statement>> true_stmts;
                    vector<ASTPointer<Statement>> false_stmts;

#if defined(CONTRACT_DEBUG)

                    // OraclizeEvent(_queryId, "sqrt", "Valid result");
                    auto log_valid_sqrt = make_shared<ExpressionStatement>(empty_loc_,
                                                                           empty_doc_,
                                                                           make_shared<FunctionCall>(empty_loc_,
                                                                                                     make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                     vector<ASTPointer<Expression>>{
                                                                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                         make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("sqrt")),
                                                                                                         make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("Valid result"))},
                                                                                                     vector<ASTPointer<ASTString>>{}));
                    true_stmts.push_back(log_valid_sqrt);

                    // OraclizeEvent(_queryId, "sqrt", "Invalid result");
                    auto log_invalid_sqrt = make_shared<ExpressionStatement>(empty_loc_,
                                                                             empty_doc_,
                                                                             make_shared<FunctionCall>(empty_loc_,
                                                                                                       make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                       vector<ASTPointer<Expression>>{
                                                                                                           make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                           make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("sqrt")),
                                                                                                           make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("Invalid result"))},
                                                                                                       vector<ASTPointer<ASTString>>{}));
                    false_stmts.push_back(log_invalid_sqrt);

#endif

                    auto true_branch = make_shared<Block>(empty_loc_, empty_doc_, true_stmts);

                    // _result = "";
                    lhs = make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result"));
                    rhs = make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(""));
                    auto reset_result = make_shared<ExpressionStatement>(empty_loc_,
                                                                         empty_doc_,
                                                                         make_shared<Assignment>(empty_loc_, lhs, Token::Assign, rhs));

                    false_stmts.push_back(reset_result);

                    auto false_branch = make_shared<Block>(empty_loc_, empty_doc_, false_stmts);

                    stmts.push_back(make_shared<IfStatement>(empty_loc_, empty_doc_, and_cond, true_branch, false_branch));
                    break;
                }
                case OracleType::ThreeSum:
                {
                    /* 
                     * if (bytes(_result).length != 0) { // no 3sum
                     *    uint[] _3sum = stringToArray(_result);
                     *    if ((_3sum.length == 3) &&
                     *        (_3sum[0] != _3sum[1] && _3sum[0] != _3sum[2] && _3sum[1] != _3sum[2]) &&
                     *        (_oEnvN.nums[_3sum[0]] + _oEnvN.nums[_3sum[1]] + _oEnvN.nums[_3sum[2]] == _oEnvN.sum)) {
                     *       OraclizeEvent(_queryId, "3sum", "Valid result");
                     *    }
                     *    else {
                     *       OraclizeEvent(_queryId, "3sum", "Invalid result");
                     *       _result = "";
                     *    }
                     * }
                     * else {
                     *    OraclizeEvent(_queryId, "3sum", "No triple of indicies summing to target sum");
                     * }
                     */

                    call_args.clear();

                    // uint[] _3sum = stringToArray(_result);
                    call_args.push_back(make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result")));

                    auto init_val = make_shared<FunctionCall>(empty_loc_,
                                                              make_shared<Identifier>(empty_loc_, make_shared<ASTString>("stringToArray")),
                                                              call_args,
                                                              vector<ASTPointer<ASTString>>{});

                    auto subtype = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
                    auto type = make_shared<ArrayTypeName>(empty_loc_, subtype, ASTPointer<Expression>());

                    auto var_decl = make_shared<VariableDeclaration>(empty_loc_,
                                                                     type,
                                                                     make_shared<ASTString>(string("_3sum") + to_string(i)),
                                                                     ASTPointer<Expression>(),
                                                                     Declaration::Visibility::Default,
                                                                     false,
                                                                     false,
                                                                     false,
                                                                     VariableDeclaration::Location::Memory);

                    auto array_var = make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                               empty_doc_,
                                                                               vector<ASTPointer<VariableDeclaration>>{var_decl},
                                                                               init_val);

                    // _3sum.length == 3
                    auto cmp_len = make_shared<BinaryOperation>(empty_loc_,
                                                                make_shared<MemberAccess>(empty_loc_,
                                                                                          make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_3sum") + to_string(i))),
                                                                                          make_shared<ASTString>("length")),
                                                                Token::Equal,
                                                                make_shared<Literal>(empty_loc_,
                                                                                     Token::Number,
                                                                                     make_shared<ASTString>("3")));

                    // _3sum[0] != _3sum[1] && _3sum[0] != _3sum[2] && _3sum[1] != _3sum[2]
                    auto num0 = make_shared<IndexAccess>(empty_loc_,
                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_3sum") + to_string(i))),
                                                         make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("0")));

                    auto num1 = make_shared<IndexAccess>(empty_loc_,
                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_3sum") + to_string(i))),
                                                         make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("1")));

                    auto num2 = make_shared<IndexAccess>(empty_loc_,
                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_3sum") + to_string(i))),
                                                         make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("2")));

                    auto cmp_neq = make_shared<BinaryOperation>(empty_loc_,
                                                                make_shared<BinaryOperation>(empty_loc_,
                                                                                             make_shared<BinaryOperation>(empty_loc_, num0, Token::NotEqual, num1),
                                                                                             Token::And,
                                                                                             make_shared<BinaryOperation>(empty_loc_, num0, Token::NotEqual, num2)),
                                                                Token::And,
                                                                make_shared<BinaryOperation>(empty_loc_, num1, Token::NotEqual, num2));

                    // _oEnvN.nums[_3sum[0]] + _oEnvN.nums[_3sum[1]] + _oEnvN.nums[_3sum[2]] == _oEnvN.sum
                    num0 = make_shared<IndexAccess>(empty_loc_,
                                                    make_shared<MemberAccess>(empty_loc_,
                                                                              make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                              make_shared<ASTString>("nums")),
                                                    make_shared<IndexAccess>(empty_loc_,
                                                                             make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_3sum") + to_string(i))),
                                                                             make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("0"))));

                    num1 = make_shared<IndexAccess>(empty_loc_,
                                                    make_shared<MemberAccess>(empty_loc_,
                                                                              make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                              make_shared<ASTString>("nums")),
                                                    make_shared<IndexAccess>(empty_loc_,
                                                                             make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_3sum") + to_string(i))),
                                                                             make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("1"))));

                    num2 = make_shared<IndexAccess>(empty_loc_,
                                                    make_shared<MemberAccess>(empty_loc_,
                                                                              make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                              make_shared<ASTString>("nums")),
                                                    make_shared<IndexAccess>(empty_loc_,
                                                                             make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_3sum") + to_string(i))),
                                                                             make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("2"))));

                    auto sum = make_shared<BinaryOperation>(empty_loc_, make_shared<BinaryOperation>(empty_loc_, num0, Token::Add, num1),
                                                            Token::Add,
                                                            num2);

                    auto cmp_sum = make_shared<BinaryOperation>(empty_loc_,
                                                                sum,
                                                                Token::Equal,
                                                                make_shared<MemberAccess>(empty_loc_,
                                                                                          make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                          make_shared<ASTString>("sum")));

                    /* 
                     * (_3sum.length == 3) &&
                     * (_3sum[0] != _3sum[1] && _3sum[0] != _3sum[2] && _3sum[1] != _3sum[2]) &&
                     * (_oEnvN.nums[_3sum[0]] + _oEnvN.nums[_3sum[1]] + _oEnvN.nums[_3sum[2]] == _oEnvN.sum)
                     */
                    auto and_cond = make_shared<BinaryOperation>(empty_loc_,
                                                                 make_shared<BinaryOperation>(empty_loc_, cmp_len, Token::And, cmp_neq),
                                                                 Token::And,
                                                                 cmp_sum);

                    vector<ASTPointer<Statement>> true_stmts;
                    vector<ASTPointer<Statement>> false_stmts;

#if defined(CONTRACT_DEBUG)

                    // OraclizeEvent(_queryId, "3sum", "Valid result")
                    auto log_valid_3sum = make_shared<ExpressionStatement>(empty_loc_,
                                                                           empty_doc_,
                                                                           make_shared<FunctionCall>(empty_loc_,
                                                                                                     make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                     vector<ASTPointer<Expression>>{
                                                                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                         make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("3sum")),
                                                                                                         make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("Valid result"))},
                                                                                                     vector<ASTPointer<ASTString>>{}));
                    true_stmts.push_back(log_valid_3sum);

                    // OraclizeEvent(_queryId, "3sum", "Invalid result")
                    auto log_invalid_3sum = make_shared<ExpressionStatement>(empty_loc_,
                                                                             empty_doc_,
                                                                             make_shared<FunctionCall>(empty_loc_,
                                                                                                       make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                       vector<ASTPointer<Expression>>{
                                                                                                           make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                           make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("3sum")),
                                                                                                           make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("Invalid result"))},
                                                                                                       vector<ASTPointer<ASTString>>{}));
                    false_stmts.push_back(log_invalid_3sum);

#endif

                    // _result = ""
                    lhs = make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result"));
                    rhs = make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(""));
                    auto reset_result = make_shared<ExpressionStatement>(empty_loc_,
                                                                         empty_doc_,
                                                                         make_shared<Assignment>(empty_loc_, lhs, Token::Assign, rhs));

                    false_stmts.push_back(reset_result);

                    auto nested_if_stmt = make_shared<IfStatement>(empty_loc_,
                                                                   empty_doc_,
                                                                   and_cond,
                                                                   make_shared<Block>(empty_loc_,
                                                                                      empty_doc_,
                                                                                      true_stmts),
                                                                   make_shared<Block>(empty_loc_,
                                                                                      empty_doc_,
                                                                                      false_stmts));

                    false_stmts.clear();

#if defined(CONTRACT_DEBUG)

                    // OraclizeEvent(_queryId, "3sum", "No triple of indicies summing to target sum")
                    auto log_no_3sum = make_shared<ExpressionStatement>(empty_loc_,
                                                                        empty_doc_,
                                                                        make_shared<FunctionCall>(empty_loc_,
                                                                                                  make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                  vector<ASTPointer<Expression>>{
                                                                                                      make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                      make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("3sum")),
                                                                                                      make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("No triple of indicies summing to target sum"))},
                                                                                                  vector<ASTPointer<ASTString>>{}));
                    false_stmts.push_back(log_no_3sum);

#endif

                    stmts.push_back(make_shared<IfStatement>(empty_loc_,
                                                             empty_doc_,
                                                             make_shared<BinaryOperation>(empty_loc_,
                                                                                          make_shared<MemberAccess>(empty_loc_,
                                                                                                                    make_shared<FunctionCall>(empty_loc_,
                                                                                                                                              make_shared<ElementaryTypeNameExpression>(empty_loc_,
                                                                                                                                                                                        ElementaryTypeNameToken(Token::Bytes, 0, 0)),
                                                                                                                                              vector<ASTPointer<Expression>>{make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result"))},
                                                                                                                                              vector<ASTPointer<ASTString>>{}),
                                                                                                                    make_shared<ASTString>("length")),
                                                                                          Token::NotEqual,
                                                                                          make_shared<Literal>(empty_loc_,
                                                                                                               Token::Number,
                                                                                                               make_shared<ASTString>("0"))),
                                                             make_shared<Block>(empty_loc_,
                                                                                empty_doc_,
                                                                                vector<ASTPointer<Statement>>{array_var, nested_if_stmt}),
                                                             make_shared<Block>(empty_loc_,
                                                                                empty_doc_,
                                                                                false_stmts)));
                    break;
                }
                case OracleType::KP:
                {
                    /*
                     * if (bytes(_result).length != 0) { // no path of criteria
                     *    uint[] memory _path = stringToArray(_result);
                     *    uint _n = babylonian(_oEnvN.w.length);
                     *    if (_oEnvN.k == _path.length - 1) {
                     *       uint _W = 0;
                     *       for (uint i = 0; i < _path.length - 1; ++i) {
                     *          _W += _oEnvN.w[_path[i] * _n + _path[i + 1]];
                     *       }
                     *       if (_oEnvN.W >= _W) {
                     *          OraclizeEvent(_queryId, "kp", "Valid result");
                     *       }
                     *       else {
                     *          OraclizeEvent(_queryId, "kp", "Invalid result: path weight");
                     *          _result = "";
                     *       }
                     *    }
                     *    else {
                     *       OraclizeEvent(_queryId, "kp", "Invalid result: path length");
                     *       _result = "";
                     *    }
                     * }
                     * else {
                     *    OraclizeEvent(_queryId, "kp", "No path satisfying criteria");
                     * }
                     */

                    ASTPointer<Expression> init_val;
                    ASTPointer<TypeName> type;
                    ASTPointer<VariableDeclaration> var_decl;

                    call_args.clear();

                    // uint[] memory _path = stringToArray(_result);
                    call_args.push_back(make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result")));

                    init_val = make_shared<FunctionCall>(empty_loc_,
                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>("stringToArray")),
                                                         call_args,
                                                         vector<ASTPointer<ASTString>>{});

                    auto subtype = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
                    type = make_shared<ArrayTypeName>(empty_loc_, subtype, ASTPointer<Expression>());

                    var_decl = make_shared<VariableDeclaration>(empty_loc_,
                                                                type,
                                                                make_shared<ASTString>(string("_path") + to_string(i)),
                                                                ASTPointer<Expression>(),
                                                                Declaration::Visibility::Default,
                                                                false,
                                                                false,
                                                                false,
                                                                VariableDeclaration::Location::Memory);

                    auto path_var = make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                              empty_doc_,
                                                                              vector<ASTPointer<VariableDeclaration>>{var_decl},
                                                                              init_val);

                    call_args.clear();

                    // uint _n = babylonian(_oEnvN.w.length);
                    call_args.push_back(make_shared<MemberAccess>(empty_loc_,
                                                                  make_shared<MemberAccess>(empty_loc_,
                                                                                            make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                            make_shared<ASTString>("w")),
                                                                  make_shared<ASTString>("length")));

                    init_val = make_shared<FunctionCall>(empty_loc_,
                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>("babylonian")),
                                                         call_args,
                                                         vector<ASTPointer<ASTString>>{});

                    type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});

                    var_decl = make_shared<VariableDeclaration>(empty_loc_,
                                                                type,
                                                                make_shared<ASTString>(string("_n") + to_string(i)),
                                                                ASTPointer<Expression>(),
                                                                Declaration::Visibility::Default);

                    auto n_var = make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                           empty_doc_,
                                                                           vector<ASTPointer<VariableDeclaration>>{var_decl},
                                                                           init_val);

                    /* 
                     * if (_oEnvN.k == _path.length - 1) {
                     *    uint _W = 0;
                     *    for (uint i = 0; i < _path.length - 1; ++i) {
                     *       ...
                     *    }
                     *    if (_oEnvN.W >= _W) {
                     *       ...
                     *    }
                     *    else {
                     *       ...
                     *    }
                     * }
                     */

                    vector<ASTPointer<Statement>> true_stmts;

                    // uint _W = 0;
                    init_val = make_shared<Literal>(empty_loc_,
                                                    Token::Number,
                                                    make_shared<ASTString>("0"));

                    type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});

                    var_decl = make_shared<VariableDeclaration>(empty_loc_,
                                                                type,
                                                                make_shared<ASTString>(string("_W") + to_string(i)),
                                                                ASTPointer<Expression>(),
                                                                Declaration::Visibility::Default);

                    true_stmts.push_back(make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                                   empty_doc_,
                                                                                   vector<ASTPointer<VariableDeclaration>>{var_decl},
                                                                                   init_val));

                    /* 
                     * for (uint i = 0; i < _path.length - 1; ++i) {
                     *    _W += _oEnvN.w[_path[i] * _n + _path[i + 1]];
                     * }
                     */

                    // uint i = 0;
                    auto for_var = make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                             empty_doc_,
                                                                             vector<ASTPointer<VariableDeclaration>>{
                                                                                 make_shared<VariableDeclaration>(empty_loc_,
                                                                                                                  make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0}),
                                                                                                                  make_shared<ASTString>(string("i") + to_string(i)),
                                                                                                                  ASTPointer<Expression>(),
                                                                                                                  Declaration::Visibility::Default)},
                                                                             make_shared<Literal>(empty_loc_,
                                                                                                  Token::Number,
                                                                                                  make_shared<ASTString>("0")));

                    // i < _path.length - 1;
                    auto for_cond = make_shared<BinaryOperation>(empty_loc_,
                                                                 make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("i") + to_string(i))),
                                                                 Token::LessThan,
                                                                 make_shared<BinaryOperation>(empty_loc_,
                                                                                              make_shared<MemberAccess>(empty_loc_,
                                                                                                                        make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_path") + to_string(i))),
                                                                                                                        make_shared<ASTString>("length")),
                                                                                              Token::Sub,
                                                                                              make_shared<Literal>(empty_loc_,
                                                                                                                   Token::Number,
                                                                                                                   make_shared<ASTString>("1"))));
                    // ++i
                    auto for_loop = make_shared<ExpressionStatement>(empty_loc_,
                                                                     empty_doc_,
                                                                     make_shared<UnaryOperation>(empty_loc_,
                                                                                                 Token::Inc,
                                                                                                 make_shared<Identifier>(empty_loc_,
                                                                                                                         make_shared<ASTString>(string("i") + to_string(i))),
                                                                                                 true));

                    // _W += _oEnvN.w[_path[i] * _n + _path[i + 1]];
                    lhs = make_shared<Identifier>(empty_loc_,
                                                  make_shared<ASTString>(string("_W") + to_string(i)));

                    rhs = make_shared<IndexAccess>(empty_loc_,
                                                   make_shared<MemberAccess>(empty_loc_,
                                                                             make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                             make_shared<ASTString>("w")),
                                                   make_shared<BinaryOperation>(empty_loc_,
                                                                                make_shared<BinaryOperation>(empty_loc_,
                                                                                                             make_shared<IndexAccess>(empty_loc_,
                                                                                                                                      make_shared<Identifier>(empty_loc_,
                                                                                                                                                              make_shared<ASTString>(string("_path") + to_string(i))),
                                                                                                                                      make_shared<Identifier>(empty_loc_,
                                                                                                                                                              make_shared<ASTString>(string("i") + to_string(i)))),
                                                                                                             Token::Mul,
                                                                                                             make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_n") + to_string(i)))),
                                                                                Token::Add,
                                                                                make_shared<IndexAccess>(empty_loc_,
                                                                                                         make_shared<Identifier>(empty_loc_,
                                                                                                                                 make_shared<ASTString>(string("_path") + to_string(i))),
                                                                                                         make_shared<BinaryOperation>(empty_loc_,
                                                                                                                                      make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("i") + to_string(i))),
                                                                                                                                      Token::Add,
                                                                                                                                      make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("1"))))));

                    auto for_body = make_shared<ExpressionStatement>(empty_loc_,
                                                                     empty_doc_,
                                                                     make_shared<Assignment>(empty_loc_, lhs, Token::AssignAdd, rhs));

                    true_stmts.push_back(make_shared<ForStatement>(empty_loc_,
                                                                   empty_doc_,
                                                                   for_var,
                                                                   for_cond,
                                                                   for_loop,
                                                                   for_body));

                    /* 
                     * if (_oEnvN.W >= _W) {
                     *    ...
                     * }
                     * else {
                     *    ...
                     * }
                     */

                    auto cond_expr = make_shared<BinaryOperation>(empty_loc_,
                                                                  make_shared<MemberAccess>(empty_loc_,
                                                                                            make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                            make_shared<ASTString>("W")),
                                                                  Token::GreaterThanOrEqual,
                                                                  make_shared<Identifier>(empty_loc_,
                                                                                          make_shared<ASTString>(string("_W") + to_string(i))));

                    vector<ASTPointer<Statement>> _true_stmts;
                    vector<ASTPointer<Statement>> _false_stmts;

#if defined(CONTRACT_DEBUG)

                    // OraclizeEvent(_queryId, "kp", "Valid result");
                    auto log_valid_kp = make_shared<ExpressionStatement>(empty_loc_,
                                                                         empty_doc_,
                                                                         make_shared<FunctionCall>(empty_loc_,
                                                                                                   make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                   vector<ASTPointer<Expression>>{
                                                                                                       make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                       make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("kp")),
                                                                                                       make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("Valid result"))},
                                                                                                   vector<ASTPointer<ASTString>>{}));
                    _true_stmts.push_back(log_valid_kp);

                    // OraclizeEvent(_queryId, "kp", "Invalid result: path weight");
                    auto log_invalid_kp = make_shared<ExpressionStatement>(empty_loc_,
                                                                           empty_doc_,
                                                                           make_shared<FunctionCall>(empty_loc_,
                                                                                                     make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                     vector<ASTPointer<Expression>>{
                                                                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                         make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("kp")),
                                                                                                         make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("Invalid result: path weight"))},
                                                                                                     vector<ASTPointer<ASTString>>{}));
                    _false_stmts.push_back(log_invalid_kp);

#endif

                    // _result = "";
                    lhs = make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result"));
                    rhs = make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(""));
                    auto reset_result = make_shared<ExpressionStatement>(empty_loc_,
                                                                         empty_doc_,
                                                                         make_shared<Assignment>(empty_loc_, lhs, Token::Assign, rhs));

                    _false_stmts.push_back(reset_result);

                    true_stmts.push_back(make_shared<IfStatement>(empty_loc_,
                                                                  empty_doc_,
                                                                  cond_expr,
                                                                  make_shared<Block>(empty_loc_,
                                                                                     empty_doc_,
                                                                                     _true_stmts),
                                                                  make_shared<Block>(empty_loc_,
                                                                                     empty_doc_,
                                                                                     _false_stmts)));

                    auto true_branch = make_shared<Block>(empty_loc_, empty_doc_, true_stmts);

                    _false_stmts.clear();

#if defined(CONTRACT_DEBUG)

                    // OraclizeEvent(_queryId, "kp", "Invalid result: path length");
                    log_invalid_kp = make_shared<ExpressionStatement>(empty_loc_,
                                                                      empty_doc_,
                                                                      make_shared<FunctionCall>(empty_loc_,
                                                                                                make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                vector<ASTPointer<Expression>>{
                                                                                                    make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                    make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("kp")),
                                                                                                    make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("Invalid result: path length"))},
                                                                                                vector<ASTPointer<ASTString>>{}));
                    _false_stmts.push_back(log_invalid_kp);

#endif

                    // _result = "";
                    _false_stmts.push_back(reset_result);

                    auto false_branch = make_shared<Block>(empty_loc_,
                                                           empty_doc_,
                                                           _false_stmts);

                    /* 
                     * if (bytes(_result).length != 0) {
                     *    uint[] _path = ...
                     *    uint _n = ...
                     *    if (_oEnvN.k == _path.length - 1) {
                     *       ...
                     *    }
                     *    else {
                     *       ...
                     *    }
                     * }
                     * else {
                     *    OraclizeEvent(_queryId, "kp", "No path satisfying criteria");
                     * }
                     */

                    _false_stmts.clear();

#if defined(CONTRACT_DEBUG)

                    // OraclizeEvent(_queryId, "kp", "No path satisfying criteria")
                    auto log_no_kp = make_shared<ExpressionStatement>(empty_loc_,
                                                                      empty_doc_,
                                                                      make_shared<FunctionCall>(empty_loc_,
                                                                                                make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                vector<ASTPointer<Expression>>{
                                                                                                    make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                    make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("kp")),
                                                                                                    make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("No path satisfying criteria"))},
                                                                                                vector<ASTPointer<ASTString>>{}));
                    _false_stmts.push_back(log_no_kp);

#endif

                    true_stmts.clear();

                    // _oEnvN.k == _path.length - 1
                    cond_expr = make_shared<BinaryOperation>(empty_loc_,
                                                             make_shared<MemberAccess>(empty_loc_,
                                                                                       make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                       make_shared<ASTString>("k")),
                                                             Token::Equal,
                                                             make_shared<BinaryOperation>(empty_loc_,
                                                                                          make_shared<MemberAccess>(empty_loc_,
                                                                                                                    make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_path") + to_string(i))),
                                                                                                                    make_shared<ASTString>("length")),
                                                                                          Token::Sub,
                                                                                          make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("1"))));

                    auto if_stmt = make_shared<IfStatement>(empty_loc_,
                                                            empty_doc_,
                                                            cond_expr,
                                                            true_branch,
                                                            false_branch);

                    true_stmts.push_back(path_var);
                    true_stmts.push_back(n_var);
                    true_stmts.push_back(if_stmt);

                    cond_expr = make_shared<BinaryOperation>(empty_loc_,
                                                             make_shared<MemberAccess>(empty_loc_,
                                                                                       make_shared<FunctionCall>(empty_loc_,
                                                                                                                 make_shared<ElementaryTypeNameExpression>(empty_loc_,
                                                                                                                                                           ElementaryTypeNameToken(Token::Bytes, 0, 0)),
                                                                                                                 vector<ASTPointer<Expression>>{make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result"))},
                                                                                                                 vector<ASTPointer<ASTString>>{}),
                                                                                       make_shared<ASTString>("length")),
                                                             Token::NotEqual,
                                                             make_shared<Literal>(empty_loc_,
                                                                                  Token::Number,
                                                                                  make_shared<ASTString>("0")));

                    stmts.push_back(make_shared<IfStatement>(empty_loc_,
                                                             empty_doc_,
                                                             cond_expr,
                                                             make_shared<Block>(empty_loc_,
                                                                                empty_doc_,
                                                                                true_stmts),
                                                             make_shared<Block>(empty_loc_,
                                                                                empty_doc_,
                                                                                _false_stmts)));
                    break;
                }
                case OracleType::KDS:
                {
                    /*
                     * if (bytes(_result).length != 0) { // no dominating set of criteria
                     *    uint[] memory _dset = stringToArray(_result);
                     *    uint _n = babylonian(_oEnvN.m.length);
                     *    if (_dset.length <= _oEnvN.k) {
                     *       bool[] memory _dominated = new bool[](_n);
                     *       for (uint v = 0; v < _dset.length; ++v) {
                     *          _dominated[_dset[v]] = true; 
                     *          for (uint u = 0; u < n; u++) {
                     *             if (_oEnvN.m[_dset[v] * _n + u] != 0) {
                     *                _dominated[u] = true; // Dominated vertex
                     *             }
                     *          }
                     *       }
                     *       for (uint i = 0; i < _n; ++i) {
                     *          if (_dominated[i] == false) {
                     *             break;
                     *          }
                     *       }
                     *       if (i >= _n) {
                     *          OraclizeEvent(_queryId, "kds", "Valid result");
                     *       }      
                     *       else {
                     *          OraclizeEvent(_queryId, "kds", "Invalid result: not a dominating set");
                     *          _result = "";
                     *       }
                     *    }
                     *    else { 
                     *       OraclizeEvent(_queryId, "kds", "Invalid result: set too large");
                     *       _result = "";
                     *    }
                     * }
                     * else {
                     *    OraclizeEvent(_queryId, "kds", "No dominating set satisfying criteria");
                     *    _result = "";
                     * }
                     */

                    ASTPointer<Expression> init_val;
                    ASTPointer<TypeName> type;
                    ASTPointer<VariableDeclaration> var_decl;

                    call_args.clear();

                    // uint[] _dset = stringToArray(_result);
                    call_args.push_back(make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result")));

                    init_val = make_shared<FunctionCall>(empty_loc_,
                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>("stringToArray")),
                                                         call_args,
                                                         vector<ASTPointer<ASTString>>{});

                    type = make_shared<ArrayTypeName>(empty_loc_,
                                                      make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0}),
                                                      ASTPointer<Expression>());

                    var_decl = make_shared<VariableDeclaration>(empty_loc_,
                                                                type,
                                                                make_shared<ASTString>(string("_dset") + to_string(i)),
                                                                ASTPointer<Expression>(),
                                                                Declaration::Visibility::Default,
                                                                false,
                                                                false,
                                                                false,
                                                                VariableDeclaration::Location::Memory);

                    auto dset_var = make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                              empty_doc_,
                                                                              vector<ASTPointer<VariableDeclaration>>{var_decl},
                                                                              init_val);

                    call_args.clear();

                    // uint _n = babylonian(_oEnvN.m.length);
                    call_args.push_back(make_shared<MemberAccess>(empty_loc_,
                                                                  make_shared<MemberAccess>(empty_loc_,
                                                                                            make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                            make_shared<ASTString>("m")),
                                                                  make_shared<ASTString>("length")));

                    init_val = make_shared<FunctionCall>(empty_loc_,
                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>("babylonian")),
                                                         call_args,
                                                         vector<ASTPointer<ASTString>>{});

                    type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});

                    var_decl = make_shared<VariableDeclaration>(empty_loc_,
                                                                type,
                                                                make_shared<ASTString>(string("_n") + to_string(i)),
                                                                ASTPointer<Expression>(),
                                                                Declaration::Visibility::Default);

                    auto n_var = make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                           empty_doc_,
                                                                           vector<ASTPointer<VariableDeclaration>>{var_decl},
                                                                           init_val);

                    vector<ASTPointer<Statement>> true_stmts;

                    call_args.clear();

                    // bool[] memory _dominated = new bool[](_n);
                    call_args.push_back(make_shared<Identifier>(empty_loc_,
                                                                make_shared<ASTString>(string("_n") + to_string(i))));

                    type = make_shared<ArrayTypeName>(empty_loc_,
                                                      make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::Bool, 0, 0}),
                                                      ASTPointer<Expression>());

                    init_val = make_shared<FunctionCall>(empty_loc_,
                                                         make_shared<NewExpression>(empty_loc_, type),
                                                         call_args,
                                                         vector<ASTPointer<ASTString>>{});

                    var_decl = make_shared<VariableDeclaration>(empty_loc_,
                                                                type,
                                                                make_shared<ASTString>(string("_dominated") + to_string(i)),
                                                                ASTPointer<Expression>(),
                                                                Declaration::Visibility::Default,
                                                                false,
                                                                false,
                                                                false,
                                                                VariableDeclaration::Location::Memory);

                    true_stmts.push_back(make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                                   empty_doc_,
                                                                                   vector<ASTPointer<VariableDeclaration>>{var_decl},
                                                                                   init_val));

                    /*               
                     * for (uint u = 0; u < _n; u++) {
                     *    if (_oEnvN.m[_dset[v] * _n + u] != 0) {
                     *       _dominated[u] = true;
                     *    }
                     * }
                     */

                    // uint u = 0;
                    auto for_var = make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                             empty_doc_,
                                                                             vector<ASTPointer<VariableDeclaration>>{
                                                                                 make_shared<VariableDeclaration>(empty_loc_,
                                                                                                                  make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0}),
                                                                                                                  make_shared<ASTString>(string("u") + to_string(i)),
                                                                                                                  ASTPointer<Expression>(),
                                                                                                                  Declaration::Visibility::Default)},
                                                                             make_shared<Literal>(empty_loc_,
                                                                                                  Token::Number,
                                                                                                  make_shared<ASTString>("0")));

                    // u < _n;
                    auto for_cond = make_shared<BinaryOperation>(empty_loc_,
                                                                 make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("u") + to_string(i))),
                                                                 Token::LessThan,
                                                                 make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_n") + to_string(i))));
                    // ++u
                    auto for_loop = make_shared<ExpressionStatement>(empty_loc_,
                                                                     empty_doc_,
                                                                     make_shared<UnaryOperation>(empty_loc_,
                                                                                                 Token::Inc,
                                                                                                 make_shared<Identifier>(empty_loc_,
                                                                                                                         make_shared<ASTString>(string("u") + to_string(i))),
                                                                                                 true));

                    /*    
                     * if (_oEnvN.m[_dset[v] * _n + u] != 0) {
                     *    _dominated[u] = true;
                     * }                    
                     */

                    // _dset[v] * _n + u
                    auto index_expr = make_shared<BinaryOperation>(empty_loc_,
                                                                   make_shared<BinaryOperation>(empty_loc_,
                                                                                                make_shared<IndexAccess>(empty_loc_,
                                                                                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_dset") + to_string(i))),
                                                                                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("v") + to_string(i)))),
                                                                                                Token::Mul,
                                                                                                make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_n") + to_string(i)))),
                                                                   Token::Add,
                                                                   make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("u") + to_string(i))));

                    // _oEnvN.m[_dset[v] * _n + u] != 0
                    auto cond_expr = make_shared<BinaryOperation>(empty_loc_,
                                                                  make_shared<IndexAccess>(empty_loc_,
                                                                                           make_shared<MemberAccess>(empty_loc_,
                                                                                                                     make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                                                     make_shared<ASTString>("m")),
                                                                                           index_expr),
                                                                  Token::NotEqual,
                                                                  make_shared<Literal>(empty_loc_,
                                                                                       Token::Number,
                                                                                       make_shared<ASTString>("0")));

                    // _dominated[u] = true;
                    lhs = make_shared<IndexAccess>(empty_loc_,
                                                   make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_dominated") + to_string(i))),
                                                   make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("u") + to_string(i))));

                    rhs = make_shared<Literal>(empty_loc_, Token::TrueLiteral, make_shared<ASTString>("true"));

                    auto assign = make_shared<ExpressionStatement>(empty_loc_,
                                                                   empty_doc_,
                                                                   make_shared<Assignment>(empty_loc_, lhs, Token::Assign, rhs));

                    auto if_stmt = make_shared<IfStatement>(empty_loc_,
                                                            empty_doc_,
                                                            cond_expr,
                                                            assign,
                                                            make_shared<Block>(empty_loc_, empty_doc_, vector<ASTPointer<Statement>>{}));

                    auto for_stmt = make_shared<ForStatement>(empty_loc_,
                                                              empty_doc_,
                                                              for_var,
                                                              for_cond,
                                                              for_loop,
                                                              if_stmt);

                    /* 
                     * for (uint v = 0; v < _dset.length; ++v) {
                     *    _dominated[_dset[v]] = true; 
                     *    for (uint u = 0; u < _n; u++) {
                     *       ...
                     *    }
                     * }
                     */

                    // uint v = 0;
                    for_var = make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                        empty_doc_,
                                                                        vector<ASTPointer<VariableDeclaration>>{
                                                                            make_shared<VariableDeclaration>(empty_loc_,
                                                                                                             make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0}),
                                                                                                             make_shared<ASTString>(string("v") + to_string(i)),
                                                                                                             ASTPointer<Expression>(),
                                                                                                             Declaration::Visibility::Default)},
                                                                        make_shared<Literal>(empty_loc_,
                                                                                             Token::Number,
                                                                                             make_shared<ASTString>("0")));

                    // v < _dset.length;
                    for_cond = make_shared<BinaryOperation>(empty_loc_,
                                                            make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("v") + to_string(i))),
                                                            Token::LessThan,
                                                            make_shared<MemberAccess>(empty_loc_,
                                                                                      make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_dset") + to_string(i))),
                                                                                      make_shared<ASTString>("length")));
                    // ++v
                    for_loop = make_shared<ExpressionStatement>(empty_loc_,
                                                                empty_doc_,
                                                                make_shared<UnaryOperation>(empty_loc_,
                                                                                            Token::Inc,
                                                                                            make_shared<Identifier>(empty_loc_,
                                                                                                                    make_shared<ASTString>(string("v") + to_string(i))),
                                                                                            true));

                    // _dominated[_dset[v]] = true;
                    lhs = make_shared<IndexAccess>(empty_loc_,
                                                   make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_dominated") + to_string(i))),
                                                   make_shared<IndexAccess>(empty_loc_,
                                                                            make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_dset") + to_string(i))),
                                                                            make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("v") + to_string(i)))));

                    rhs = make_shared<Literal>(empty_loc_, Token::TrueLiteral, make_shared<ASTString>("true"));

                    assign = make_shared<ExpressionStatement>(empty_loc_,
                                                              empty_doc_,
                                                              make_shared<Assignment>(empty_loc_, lhs, Token::Assign, rhs));

                    for_stmt = make_shared<ForStatement>(empty_loc_,
                                                         empty_doc_,
                                                         for_var,
                                                         for_cond,
                                                         for_loop,
                                                         make_shared<Block>(empty_loc_,
                                                                            empty_doc_,
                                                                            vector<ASTPointer<Statement>>{assign, for_stmt}));
                    true_stmts.push_back(for_stmt);

                    /*
                     * for (uint i = 0; i < _n; ++i) {
                     *    if (_dominated[i] == false) {
                     *       break;
                     *    }
                     * }
                     */

                    // _dominated[i] == false
                    cond_expr = make_shared<BinaryOperation>(empty_loc_,
                                                             make_shared<IndexAccess>(empty_loc_,
                                                                                      make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_dominated") + to_string(i))),
                                                                                      make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("i") + to_string(i)))),
                                                             Token::Equal,
                                                             make_shared<Literal>(empty_loc_, Token::FalseLiteral, make_shared<ASTString>("false")));

                    if_stmt = make_shared<IfStatement>(empty_loc_,
                                                       empty_doc_,
                                                       cond_expr,
                                                       make_shared<Break>(empty_loc_, empty_doc_),
                                                       make_shared<Block>(empty_loc_, empty_doc_, vector<ASTPointer<Statement>>{}));

                    // uint i = 0;
                    for_var = make_shared<VariableDeclarationStatement>(empty_loc_,
                                                                        empty_doc_,
                                                                        vector<ASTPointer<VariableDeclaration>>{
                                                                            make_shared<VariableDeclaration>(empty_loc_,
                                                                                                             make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0}),
                                                                                                             make_shared<ASTString>(string("i") + to_string(i)),
                                                                                                             ASTPointer<Expression>(),
                                                                                                             Declaration::Visibility::Default)},
                                                                        make_shared<Literal>(empty_loc_,
                                                                                             Token::Number,
                                                                                             make_shared<ASTString>("0")));

                    // i < _n;
                    for_cond = make_shared<BinaryOperation>(empty_loc_,
                                                            make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("i") + to_string(i))),
                                                            Token::LessThan,
                                                            make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_n") + to_string(i))));

                    // ++i
                    for_loop = make_shared<ExpressionStatement>(empty_loc_,
                                                                empty_doc_,
                                                                make_shared<UnaryOperation>(empty_loc_,
                                                                                            Token::Inc,
                                                                                            make_shared<Identifier>(empty_loc_,
                                                                                                                    make_shared<ASTString>(string("i") + to_string(i))),
                                                                                            true));

                    for_stmt = make_shared<ForStatement>(empty_loc_,
                                                         empty_doc_,
                                                         for_var,
                                                         for_cond,
                                                         for_loop,
                                                         if_stmt);

                    true_stmts.push_back(for_stmt);

                    /*
                     * if (i >= _n) {
                     *    OraclizeEvent(_queryId, "kds", "Valid result");
                     * }      
                     * else {
                     *    OraclizeEvent(_queryId, "kds", "Invalid result: not a dominating set");
                     *    _result = "";
                     * }
                     */

                    // i >= _n
                    cond_expr = make_shared<BinaryOperation>(empty_loc_,
                                                             make_shared<Identifier>(empty_loc_,
                                                                                     make_shared<ASTString>(string("i") + to_string(i))),
                                                             Token::GreaterThanOrEqual,
                                                             make_shared<Identifier>(empty_loc_,
                                                                                     make_shared<ASTString>(string("_n") + to_string(i))));

                    vector<ASTPointer<Statement>> _true_stmts;
                    vector<ASTPointer<Statement>> _false_stmts;

#if defined(CONTRACT_DEBUG)

                    // OraclizeEvent(_queryId, "kds", "Valid result");
                    auto log_valid_kds = make_shared<ExpressionStatement>(empty_loc_,
                                                                          empty_doc_,
                                                                          make_shared<FunctionCall>(empty_loc_,
                                                                                                    make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                    vector<ASTPointer<Expression>>{
                                                                                                        make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                        make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("kds")),
                                                                                                        make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("Valid result"))},
                                                                                                    vector<ASTPointer<ASTString>>{}));

                    _true_stmts.push_back(log_valid_kds);

                    // OraclizeEvent(_queryId, "kds", "Invalid result: not a dominating set");
                    auto log_invalid_kds = make_shared<ExpressionStatement>(empty_loc_,
                                                                            empty_doc_,
                                                                            make_shared<FunctionCall>(empty_loc_,
                                                                                                      make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                      vector<ASTPointer<Expression>>{
                                                                                                          make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                          make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("kds")),
                                                                                                          make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("Invalid result: not a dominating set"))},
                                                                                                      vector<ASTPointer<ASTString>>{}));

                    _false_stmts.push_back(log_invalid_kds);

#endif

                    // _result = "";
                    lhs = make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result"));
                    rhs = make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(""));
                    auto reset_result = make_shared<ExpressionStatement>(empty_loc_,
                                                                         empty_doc_,
                                                                         make_shared<Assignment>(empty_loc_, lhs, Token::Assign, rhs));

                    _false_stmts.push_back(reset_result);

                    if_stmt = make_shared<IfStatement>(empty_loc_,
                                                       empty_doc_,
                                                       cond_expr,
                                                       make_shared<Block>(empty_loc_, empty_doc_, _true_stmts),
                                                       make_shared<Block>(empty_loc_, empty_doc_, _false_stmts));

                    true_stmts.push_back(if_stmt);

                    /* 
                     * if (_dset.length <= _oEnvN.k) {
                     *    ...   
                     * }
                     * else { 
                     *    OraclizeEvent(_queryId, "kds", "Invalid result: set too large");
                     *    _result = "";
                     * }
                     */

                    // _dset.length <= _oEnvN.k
                    cond_expr = make_shared<BinaryOperation>(empty_loc_,
                                                             make_shared<MemberAccess>(empty_loc_,
                                                                                       make_shared<Identifier>(empty_loc_, make_shared<ASTString>(string("_dset") + to_string(i))),
                                                                                       make_shared<ASTString>("length")),
                                                             Token::LessThanOrEqual,
                                                             make_shared<MemberAccess>(empty_loc_,
                                                                                       make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                       make_shared<ASTString>("k")));

                    _false_stmts.clear();

#if defined(CONTRACT_DEBUG)

                    // OraclizeEvent(_queryId, "kds", "Invalid result: set too large");
                    log_invalid_kds = make_shared<ExpressionStatement>(empty_loc_,
                                                                       empty_doc_,
                                                                       make_shared<FunctionCall>(empty_loc_,
                                                                                                 make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                 vector<ASTPointer<Expression>>{
                                                                                                     make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                     make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("kds")),
                                                                                                     make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("Invalid result: set too large"))},
                                                                                                 vector<ASTPointer<ASTString>>{}));

                    _false_stmts.push_back(log_invalid_kds);

#endif

                    // _result = "";
                    _false_stmts.push_back(reset_result);

                    if_stmt = make_shared<IfStatement>(empty_loc_,
                                                       empty_doc_,
                                                       cond_expr,
                                                       make_shared<Block>(empty_loc_, empty_doc_, true_stmts),
                                                       make_shared<Block>(empty_loc_, empty_doc_, _false_stmts));

                    true_stmts.clear();

                    true_stmts.push_back(dset_var);
                    true_stmts.push_back(n_var);
                    true_stmts.push_back(if_stmt);

                    /*
                     * if (bytes(_result).length != 0) { // no dominating set of criteria
                     *    ...
                     * }
                     * else {
                     *    OraclizeEvent(_queryId, "kds", "No dominating set satisfying criteria");
                     *    _result = "";
                     * }
                     */

                    _false_stmts.clear();

#if defined(CONTRACT_DEBUG)

                    // OraclizeEvent(_queryId, "kds", "No dominating set satisfying criteria");
                    auto log_no_kds = make_shared<ExpressionStatement>(empty_loc_,
                                                                       empty_doc_,
                                                                       make_shared<FunctionCall>(empty_loc_,
                                                                                                 make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                                 vector<ASTPointer<Expression>>{
                                                                                                     make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                                     make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("kds")),
                                                                                                     make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("No dominating set satisfying criteria"))},
                                                                                                 vector<ASTPointer<ASTString>>{}));

                    _false_stmts.push_back(log_no_kds);

#endif

                    // _result = "";
                    _false_stmts.push_back(reset_result);

                    cond_expr = make_shared<BinaryOperation>(empty_loc_,
                                                             make_shared<MemberAccess>(empty_loc_,
                                                                                       make_shared<FunctionCall>(empty_loc_,
                                                                                                                 make_shared<ElementaryTypeNameExpression>(empty_loc_,
                                                                                                                                                           ElementaryTypeNameToken(Token::Bytes, 0, 0)),
                                                                                                                 vector<ASTPointer<Expression>>{make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result"))},
                                                                                                                 vector<ASTPointer<ASTString>>{}),
                                                                                       make_shared<ASTString>("length")),
                                                             Token::NotEqual,
                                                             make_shared<Literal>(empty_loc_,
                                                                                  Token::Number,
                                                                                  make_shared<ASTString>("0")));

                    stmts.push_back(make_shared<IfStatement>(empty_loc_,
                                                             empty_doc_,
                                                             cond_expr,
                                                             make_shared<Block>(empty_loc_, empty_doc_, true_stmts),
                                                             make_shared<Block>(empty_loc_, empty_doc_, _false_stmts)));
                    break;
                }
                default:
                {
                    throw OraclizeSolidityException("OraclizeTransform::OraclizeCallback: : Unknown verifiable query type.");
                }
                }
            }

            call_args.clear();

            // callback(_result);
            call_args.push_back(make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result")));

            stmts.push_back(make_shared<ExpressionStatement>(empty_loc_,
                                                             empty_doc_,
                                                             make_shared<FunctionCall>(empty_loc_,
                                                                                       oracle_query->callback(),
                                                                                       call_args,
                                                                                       vector<ASTPointer<ASTString>>{})));
            // delete _oEnvN;
            stmts.push_back(make_shared<ExpressionStatement>(empty_loc_,
                                                             empty_doc_,
                                                             make_shared<UnaryOperation>(empty_loc_,
                                                                                         Token::Delete,
                                                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                                         true)));
            break;
        }
        default:
        {
            throw OraclizeSolidityException("OraclizeTransform::OraclizeCallback: : Unknown verifiable query type.");
        }
        }

        /* 
         * if (_queryId == _oEnvN.queryId) {
         *    ... 
         * }
         */

        // _queryId == _oEnvN.queryId
        auto cond = make_shared<BinaryOperation>(empty_loc_,
                                                 make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                 Token::Equal,
                                                 make_shared<MemberAccess>(empty_loc_,
                                                                           make_shared<Identifier>(empty_loc_, make_shared<ASTString>(oracle_query->var_name())),
                                                                           make_shared<ASTString>("queryId")));

        if (first_iteration)
        {

            first_iteration = false;

            current_if_stmt = make_shared<IfStatement>(empty_loc_,
                                                       empty_doc_,
                                                       cond,
                                                       make_shared<Block>(empty_loc_, empty_doc_, stmts),
                                                       make_shared<Block>(empty_loc_, empty_doc_, vector<ASTPointer<Statement>>{})); // Empty block
        }
        else
        {
            current_if_stmt = make_shared<IfStatement>(empty_loc_,
                                                       empty_doc_,
                                                       cond,
                                                       make_shared<Block>(empty_loc_, empty_doc_, stmts),
                                                       make_shared<Block>(empty_loc_, empty_doc_, vector<ASTPointer<Statement>>{current_if_stmt}));
        }

        i++;
    }

    /* 
     * _callback(bytes32 _queryId, string _result) {
     *    ... 
     * }
     */

    vector<ASTPointer<Statement>> body_stmts;

#if defined(CONTRACT_DEBUG)

    // OraclizeEvent(_queryId, "", "_callback() received:")
    auto log_callback = make_shared<ExpressionStatement>(empty_loc_,
                                                         empty_doc_,
                                                         make_shared<FunctionCall>(empty_loc_,
                                                                                   make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                   vector<ASTPointer<Expression>>{
                                                                                       make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                       make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("")),
                                                                                       make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("__callback() received:"))},
                                                                                   vector<ASTPointer<ASTString>>{}));
    body_stmts.push_back(log_callback);

    // OraclizeEvent(_queryId, "", _result)
    auto log_result = make_shared<ExpressionStatement>(empty_loc_,
                                                       empty_doc_,
                                                       make_shared<FunctionCall>(empty_loc_,
                                                                                 make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                 vector<ASTPointer<Expression>>{
                                                                                     make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_queryId")),
                                                                                     make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("")),
                                                                                     make_shared<Identifier>(empty_loc_, make_shared<ASTString>("_result"))},
                                                                                 vector<ASTPointer<ASTString>>{}));
    body_stmts.push_back(log_result);

#endif

    body_stmts.push_back(current_if_stmt);

    ASTPointer<TypeName> type;
    vector<ASTPointer<VariableDeclaration>> params;

    // bytes32 _queryId
    type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::BytesM, 32, 0});
    params.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("_queryId"),
                                                      shared_ptr<Expression>(), Declaration::Visibility::Default));

    // string _result
    type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::String, 0, 0});
    params.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("_result"),
                                                      shared_ptr<Expression>(), Declaration::Visibility::Default));

    ASTPointer<Block> body = make_shared<Block>(empty_loc_,
                                                empty_doc_,
                                                body_stmts);

    auto callback = make_shared<FunctionDefinition>(empty_loc_,
                                                    make_shared<ASTString>("__callback"),
                                                    Declaration::Visibility::Public,
                                                    StateMutability::NonPayable,
                                                    false,
                                                    empty_doc_,
                                                    make_shared<ParameterList>(empty_loc_, params),
                                                    vector<ASTPointer<ModifierInvocation>>{},
                                                    make_shared<ParameterList>(empty_loc_, vector<ASTPointer<VariableDeclaration>>{}),
                                                    body);

    sub_nodes.insert(sub_nodes.end(), callback);

    return true;
}

ASTPointer<StructDefinition> OraclizeTransform::EnvironmentDef(const ASTPointer<ASTString> &name, shared_ptr<OracleQuery> oracle_query)
{
    /* 
     * struct OEnvN
     * {
     *    string[nURL] urls;      // "data" only 
     *    string[nURL] retVals;   // "data" only
     *    uint8 index;            // "data" only
     *    bytes32 queryId;
     *    [Verification]
     * }
     */

    ASTPointer<TypeName> type;
    ASTPointer<TypeName> sub_type;
    ASTPointer<Expression> len;

    vector<ASTPointer<VariableDeclaration>> members;

    if (oracle_query->type() == OracleType::Data && oracle_query->QuerySize() > 1)
    {
        // string[n] urls
        sub_type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::String, 0, 0});
        len = make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>(to_string(oracle_query->QuerySize())));

        type = make_shared<ArrayTypeName>(empty_loc_, sub_type, len);

        members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("urls"),
                                                           shared_ptr<Expression>(), Declaration::Visibility::Default));

        // string[n] retVals
        sub_type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::String, 0, 0});
        len = make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>(to_string(oracle_query->QuerySize())));

        type = make_shared<ArrayTypeName>(empty_loc_, sub_type, len);

        members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("retVals"),
                                                           shared_ptr<Expression>(), Declaration::Visibility::Default));

        // uint8 index
        type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UIntM, 8, 0});

        members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("index"),
                                                           shared_ptr<Expression>(), Declaration::Visibility::Default));
    }

    // bytes32 queryId
    type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::BytesM, 32, 0});

    members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("queryId"),
                                                       shared_ptr<Expression>(), Declaration::Visibility::Default));

    // [Verification]
    auto query = dynamic_cast<VerifierQuery *>(oracle_query.get());
    if (query && query->verify())
    {
        switch (query->type())
        {
        case OracleType::Sqrt:
        {
            // uint sqrt
            type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});

            members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("sqrt"),
                                                               shared_ptr<Expression>(), Declaration::Visibility::Default));
            break;
        }
        case OracleType::ThreeSum:
        {
#if defined(UINT_256)
            sub_type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
#else
            sub_type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UIntM, THREESUM_UINT_X, 0});
#endif
            // uint[] nums
            type = make_shared<ArrayTypeName>(empty_loc_, sub_type, ASTPointer<Expression>());
            members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("nums"),
                                                               shared_ptr<Expression>(), Declaration::Visibility::Default));

#if defined(UINT_256)
            type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
#else
            type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UIntM, THREESUM_UINT_X * 2, 0});
#endif
            // uint sum
            members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("sum"),
                                                               shared_ptr<Expression>(), Declaration::Visibility::Default));
            break;
        }
        case OracleType::KP:
        {
#if defined(UINT_256)
            sub_type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
#else
            sub_type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UIntM, KP_UINT_X, 0});
#endif
            // uint[] w
            type = make_shared<ArrayTypeName>(empty_loc_, sub_type, ASTPointer<Expression>());
            members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("w"),
                                                               shared_ptr<Expression>(), Declaration::Visibility::Default));

#if defined(UINT_256)
            type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
#else
            type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
#endif
            // uint k
            members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("k"),
                                                               shared_ptr<Expression>(), Declaration::Visibility::Default));

#if defined(UINT_256)
            type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
#else
            type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
#endif
            // uint W
            members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("W"),
                                                               shared_ptr<Expression>(), Declaration::Visibility::Default));
            break;
        }
        case OracleType::KDS:
        {
#if defined(UINT_256)
            sub_type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
#else
            sub_type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UIntM, KDS_UINT_X, 0});
#endif
            // uint[] m
            type = make_shared<ArrayTypeName>(empty_loc_, sub_type, ASTPointer<Expression>());
            members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("m"),
                                                               shared_ptr<Expression>(), Declaration::Visibility::Default));

#if defined(UINT_256)
            type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
#else
            type = make_shared<ElementaryTypeName>(empty_loc_, ElementaryTypeNameToken{Token::UInt, 0, 0});
#endif
            // uint k
            members.push_back(make_shared<VariableDeclaration>(empty_loc_, type, make_shared<ASTString>("k"),
                                                               shared_ptr<Expression>(), Declaration::Visibility::Default));
            break;
        }
        default:
        {
            throw OraclizeSolidityException("OraclizeTransform::EnvironmentDef: Unknown verifiable query type.");
        }
        }
    }

    return make_shared<StructDefinition>(empty_loc_, name, members);
}

ASTPointer<Assignment> OraclizeTransform::EnvironmentInit(shared_ptr<OracleQuery> oracle_query)
{
    /*
     * _oEnvN = OEnvN(...)
     */

    vector<ASTPointer<Expression>> call_args;

    if (oracle_query->type() == OracleType::Data && oracle_query->QuerySize() > 1)
    {
        // Recursive "data" oracle query
        vector<ASTPointer<Expression>> urls;
        vector<ASTPointer<Expression>> results;
        DataQuery *data_query = dynamic_cast<DataQuery *>(oracle_query.get());
        for (auto url : data_query->urls())
        {
            urls.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(url)));
            results.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("")));
        }

        call_args.push_back(make_shared<TupleExpression>(empty_loc_, urls, true));                         // urls
        call_args.push_back(make_shared<TupleExpression>(empty_loc_, results, true));                      // retVals
        call_args.push_back(make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("0"))); // index
    }

    call_args.push_back(make_shared<FunctionCall>(empty_loc_, // queryId
                                                  make_shared<ElementaryTypeNameExpression>(empty_loc_, ElementaryTypeNameToken{Token::BytesM, 32, 0}),
                                                  vector<ASTPointer<Expression>>{make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("0"))},
                                                  vector<ASTPointer<ASTString>>{}));

    // [Verification]
    auto query = dynamic_cast<VerifierQuery *>(oracle_query.get());
    if (query && query->verify())
    {
        switch (query->type())
        {
        case OracleType::Sqrt:
        {
            auto query = dynamic_cast<SqrtQuery *>(oracle_query.get());

            // uint sqrt
            call_args.push_back(query->expression());
            break;
        }
        case OracleType::ThreeSum:
        {
            auto query = dynamic_cast<ThreeSumQuery *>(oracle_query.get());

            // uint[] nums
            call_args.push_back(query->expression());

            // uint sum
            call_args.push_back(query->sum());
            break;
        }
        case OracleType::KP:
        {
            auto query = dynamic_cast<KPQuery *>(oracle_query.get());

            // uint[] w
            call_args.push_back(query->expression());

            // uint k
            call_args.push_back(query->path_len());

            // uint W
            call_args.push_back(query->max_path_weight());
            break;
        }
        case OracleType::KDS:
        {
            auto query = dynamic_cast<KDSQuery *>(oracle_query.get());

            // uint[] m
            call_args.push_back(query->expression());

            // uint k
            call_args.push_back(query->max_size());
            break;
        }
        default:
        {
            throw OraclizeSolidityException("OraclizeTransform::EnvironmentInit: : Unknown verifiable query type.");
        }
        }
    }

    auto assign = make_shared<Assignment>(empty_loc_,
                                          make_shared<Identifier>(empty_loc_,
                                                                  make_shared<ASTString>(oracle_query->var_name())),
                                          Token::Assign,
                                          make_shared<FunctionCall>(empty_loc_,
                                                                    make_shared<Identifier>(empty_loc_,
                                                                                            make_shared<ASTString>(oracle_query->env_name())),
                                                                    call_args,
                                                                    vector<ASTPointer<ASTString>>{}));

    return assign;
}

pair<ASTPointer<FunctionCall>, ASTPointer<Expression>> OraclizeTransform::OraclizeQuery(shared_ptr<OracleQuery> oracle_query)
{
    /* 
     * oraclize_query("URL", _oEnvN.urls[_oEnvN.index]) // "data" only
     * oraclize_query("URL", url) // "other"
     */

    ASTPointer<Expression> url;
    vector<ASTPointer<Expression>> call_args;

    call_args.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>("URL")));

    switch (oracle_query->type())
    {
    case OracleType::Data:
    {
        auto *query = dynamic_cast<DataQuery *>(oracle_query.get());
        const auto query_size = query->QuerySize();

        if (query_size == 1)
        {
            url = make_shared<Literal>(empty_loc_,
                                       Token::StringLiteral,
                                       make_shared<ASTString>(query->urls()[0]));
            call_args.push_back(url);
        }
        else
        {
            url = make_shared<IndexAccess>(empty_loc_,
                                           make_shared<MemberAccess>(empty_loc_,
                                                                     make_shared<Identifier>(empty_loc_, make_shared<ASTString>(query->var_name())),
                                                                     make_shared<ASTString>("urls")),
                                           make_shared<MemberAccess>(empty_loc_,
                                                                     make_shared<Identifier>(empty_loc_, make_shared<ASTString>(query->var_name())),
                                                                     make_shared<ASTString>("index")));
            call_args.push_back(url);
        }
        break;
    }
    case OracleType::Min:
    case OracleType::Sort:
    case OracleType::APSP:
    {
        auto *query = dynamic_cast<ExpressionQuery *>(oracle_query.get());
        string prefixUrl = query->url().empty() ? oracleTypeToURL.at(query->type()) : query->url();
        string delim = "/";

        vector<ASTPointer<Expression>> _call_args;

        _call_args.push_back(query->expression());
        _call_args.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(delim)));
        _call_args.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(prefixUrl)));

        // Construct URL dynamically at runtime
        url = make_shared<FunctionCall>(empty_loc_,
                                        make_shared<Identifier>(empty_loc_, make_shared<ASTString>("arrayToStringWithPrefix")),
                                        _call_args,
                                        vector<ASTPointer<ASTString>>{});
        call_args.push_back(url);
        break;
    }
    case OracleType::ThreeSum:
    {
        auto *query = dynamic_cast<ThreeSumQuery *>(oracle_query.get());
        string prefixUrl = query->url().empty() ? oracleTypeToURL.at(query->type()) : query->url();
        string delim = "/";

        vector<ASTPointer<Expression>> _call_args;

        _call_args.push_back(query->sum());
        _call_args.push_back(query->expression());
        _call_args.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(delim)));
        _call_args.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(prefixUrl)));

        // Construct URL dynamically at runtime
        url = make_shared<FunctionCall>(empty_loc_,
                                        make_shared<Identifier>(empty_loc_, make_shared<ASTString>("uintAndArrayToStringWithPrefix")),
                                        _call_args,
                                        vector<ASTPointer<ASTString>>{});
        call_args.push_back(url);
        break;
    }
    case OracleType::Sqrt:
    {
        auto *query = dynamic_cast<SqrtQuery *>(oracle_query.get());
        string prefixUrl = query->url().empty() ? oracleTypeToURL.at(query->type()) : query->url();

        vector<ASTPointer<Expression>> _call_args;

        _call_args.push_back(query->expression());
        _call_args.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(prefixUrl)));

        // Construct URL dynamically at runtime
        url = make_shared<FunctionCall>(empty_loc_,
                                        make_shared<Identifier>(empty_loc_, make_shared<ASTString>("uintToStringWithPrefix")),
                                        _call_args,
                                        vector<ASTPointer<ASTString>>{});
        call_args.push_back(url);
        break;
    }
    case OracleType::KP:
    {
        auto *query = dynamic_cast<KPQuery *>(oracle_query.get());
        string prefixUrl = query->url().empty() ? oracleTypeToURL.at(query->type()) : query->url();
        string delim = "/";

        vector<ASTPointer<Expression>> _call_args;

        _call_args.push_back(query->path_len());
        _call_args.push_back(query->max_path_weight());
        _call_args.push_back(query->expression());
        _call_args.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(delim)));
        _call_args.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(prefixUrl)));

        // Construct URL dynamically at runtime
        url = make_shared<FunctionCall>(empty_loc_,
                                        make_shared<Identifier>(empty_loc_, make_shared<ASTString>("uintsAndArrayToStringWithPrefix")),
                                        _call_args,
                                        vector<ASTPointer<ASTString>>{});
        call_args.push_back(url);
        break;
    }
    case OracleType::KDS:
    {
        auto *query = dynamic_cast<KDSQuery *>(oracle_query.get());
        string prefixUrl = query->url().empty() ? oracleTypeToURL.at(query->type()) : query->url();
        string delim = "/";

        vector<ASTPointer<Expression>> _call_args;

        _call_args.push_back(query->max_size());
        _call_args.push_back(query->expression());
        _call_args.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(delim)));
        _call_args.push_back(make_shared<Literal>(empty_loc_, Token::StringLiteral, make_shared<ASTString>(prefixUrl)));

        // Construct URL dynamically at runtime
        url = make_shared<FunctionCall>(empty_loc_,
                                        make_shared<Identifier>(empty_loc_, make_shared<ASTString>("uintAndArrayToStringWithPrefix")),
                                        _call_args,
                                        vector<ASTPointer<ASTString>>{});
        call_args.push_back(url);
        break;
    }
    default:
    {
        throw OraclizeSolidityException("OraclizeTransform::OraclizeQuery: : Unknown query type.");
    }
    }

    if (gas_limit_)
    {
        // User-specified gas limit (default: 200000)
        call_args.push_back(make_shared<Literal>(empty_loc_,
                                                 Token::Number,
                                                 make_shared<ASTString>(to_string(gas_limit_))));
    }

    auto oraclize_query = make_shared<FunctionCall>(empty_loc_,
                                                    make_shared<Identifier>(empty_loc_, make_shared<ASTString>("oraclize_query")),
                                                    call_args,
                                                    vector<ASTPointer<ASTString>>{});

    return make_pair(oraclize_query, url);
}

ASTPointer<Statement> OraclizeTransform::DynamicSwitching(ASTPointer<Statement> stmt, shared_ptr<OracleQuery> oracle_query)
{
    switch (oracle_query->type())
    {
    case OracleType::KP:
    {
        auto query = dynamic_cast<KPQuery *>(oracle_query.get());

        if (query->switch_func())
        {
            /* 
             * if (path_len < 3) {
             *    OraclizeEvent(bytes32(0), "kp", "switch");
             *    callback(switch_func(...)); 
             * } 
             * else {
             *    OraclizeEvent(bytes32(0), "kp", "oraclize");
             *    _oEnvN.queryId = oraclize_query("URL", url)
             *    OraclizeEvent(_oEnvN.queryId, queryType, url);
             * }
             */

            vector<ASTPointer<Expression>> call_args;

            call_args.push_back(query->expression());
            call_args.push_back(query->path_len());
            call_args.push_back(query->max_path_weight());

            // callback(switch_func(...));
            auto call = make_shared<FunctionCall>(empty_loc_,
                                                  query->callback(),
                                                  vector<ASTPointer<Expression>>{make_shared<FunctionCall>(empty_loc_,
                                                                                                           query->switch_func(),
                                                                                                           call_args,
                                                                                                           vector<ASTPointer<ASTString>>{})},
                                                  vector<ASTPointer<ASTString>>{});

            // path_len < 3
            auto cond = make_shared<BinaryOperation>(empty_loc_,
                                                     query->path_len(),
                                                     Token::LessThan,
                                                     make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("3")));

            vector<ASTPointer<Statement>> true_stmts;
            vector<ASTPointer<Statement>> false_stmts;

#if defined(CONTRACT_DEBUG)

            // OraclizeEvent(bytes32(0), "kp", "switch");
            auto log_switch = make_shared<ExpressionStatement>(empty_loc_,
                                                               empty_doc_,
                                                               make_shared<FunctionCall>(empty_loc_,
                                                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                         vector<ASTPointer<Expression>>{
                                                                                             make_shared<FunctionCall>(empty_loc_,
                                                                                                                       make_shared<ElementaryTypeNameExpression>(empty_loc_, ElementaryTypeNameToken{Token::BytesM, 32, 0}),
                                                                                                                       vector<ASTPointer<Expression>>{make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("0"))},
                                                                                                                       vector<ASTPointer<ASTString>>{}),
                                                                                             make_shared<Literal>(empty_loc_,
                                                                                                                  Token::StringLiteral,
                                                                                                                  make_shared<ASTString>(oracleTypeToString.at(query->type()))),
                                                                                             make_shared<Literal>(empty_loc_,
                                                                                                                  Token::StringLiteral,
                                                                                                                  make_shared<ASTString>("switch"))},
                                                                                         vector<ASTPointer<ASTString>>{}));

            true_stmts.push_back(log_switch);

            // OraclizeEvent(bytes32(0), "kp", "oraclize");
            auto log_oraclize = make_shared<ExpressionStatement>(empty_loc_,
                                                                 empty_doc_,
                                                                 make_shared<FunctionCall>(empty_loc_,
                                                                                           make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                           vector<ASTPointer<Expression>>{
                                                                                               make_shared<FunctionCall>(empty_loc_,
                                                                                                                         make_shared<ElementaryTypeNameExpression>(empty_loc_, ElementaryTypeNameToken{Token::BytesM, 32, 0}),
                                                                                                                         vector<ASTPointer<Expression>>{make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("0"))},
                                                                                                                         vector<ASTPointer<ASTString>>{}),
                                                                                               make_shared<Literal>(empty_loc_,
                                                                                                                    Token::StringLiteral,
                                                                                                                    make_shared<ASTString>(oracleTypeToString.at(oracle_query->type()))),
                                                                                               make_shared<Literal>(empty_loc_,
                                                                                                                    Token::StringLiteral,
                                                                                                                    make_shared<ASTString>("oraclize"))},
                                                                                           vector<ASTPointer<ASTString>>{}));

            false_stmts.push_back(log_oraclize);

#endif

            true_stmts.push_back(make_shared<ExpressionStatement>(empty_loc_, empty_doc_, call));
            false_stmts.push_back(stmt);

            auto if_stmt = make_shared<IfStatement>(empty_loc_,
                                                    empty_doc_,
                                                    cond,
                                                    make_shared<Block>(empty_loc_,
                                                                       empty_doc_,
                                                                       true_stmts),
                                                    make_shared<Block>(empty_loc_,
                                                                       empty_doc_,
                                                                       false_stmts));

            return if_stmt;
        }

        return stmt; // No decoration
    }
    case OracleType::KDS:
    {
        auto query = dynamic_cast<KDSQuery *>(oracle_query.get());

        if (query->switch_func())
        {
            /* 
             * if (max_size < 3) {
             *    OraclizeEvent(bytes32(0), "kds", "switch");
             *    callback(switch_func(...)); 
             * } 
             * else {
             *    OraclizeEvent(bytes32(0), "kds", "oraclize");
             *    _oEnvN.queryId = oraclize_query("URL", url)
             *    OraclizeEvent(_oEnvN.queryId, queryType, url);
             * }
             */

            vector<ASTPointer<Expression>> call_args;

            call_args.push_back(query->expression());
            call_args.push_back(query->max_size());

            // callback(switch_func(...));
            auto call = make_shared<FunctionCall>(empty_loc_,
                                                  query->callback(),
                                                  vector<ASTPointer<Expression>>{make_shared<FunctionCall>(empty_loc_,
                                                                                                           query->switch_func(),
                                                                                                           call_args,
                                                                                                           vector<ASTPointer<ASTString>>{})},
                                                  vector<ASTPointer<ASTString>>{});

            // max_size < 3
            auto cond = make_shared<BinaryOperation>(empty_loc_,
                                                     query->max_size(),
                                                     Token::LessThan,
                                                     make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("3")));

            vector<ASTPointer<Statement>> true_stmts;
            vector<ASTPointer<Statement>> false_stmts;

#if defined(CONTRACT_DEBUG)

            // OraclizeEvent(bytes32(0), "kds", "switch");
            auto log_switch = make_shared<ExpressionStatement>(empty_loc_,
                                                               empty_doc_,
                                                               make_shared<FunctionCall>(empty_loc_,
                                                                                         make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                         vector<ASTPointer<Expression>>{
                                                                                             make_shared<FunctionCall>(empty_loc_,
                                                                                                                       make_shared<ElementaryTypeNameExpression>(empty_loc_, ElementaryTypeNameToken{Token::BytesM, 32, 0}),
                                                                                                                       vector<ASTPointer<Expression>>{make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("0"))},
                                                                                                                       vector<ASTPointer<ASTString>>{}),
                                                                                             make_shared<Literal>(empty_loc_,
                                                                                                                  Token::StringLiteral,
                                                                                                                  make_shared<ASTString>(oracleTypeToString.at(query->type()))),
                                                                                             make_shared<Literal>(empty_loc_,
                                                                                                                  Token::StringLiteral,
                                                                                                                  make_shared<ASTString>("switch"))},
                                                                                         vector<ASTPointer<ASTString>>{}));

            true_stmts.push_back(log_switch);

            // OraclizeEvent(bytes32(0), "kds", "oraclize");
            auto log_oraclize = make_shared<ExpressionStatement>(empty_loc_,
                                                                 empty_doc_,
                                                                 make_shared<FunctionCall>(empty_loc_,
                                                                                           make_shared<Identifier>(empty_loc_, make_shared<ASTString>("OraclizeEvent")),
                                                                                           vector<ASTPointer<Expression>>{
                                                                                               make_shared<FunctionCall>(empty_loc_,
                                                                                                                         make_shared<ElementaryTypeNameExpression>(empty_loc_, ElementaryTypeNameToken{Token::BytesM, 32, 0}),
                                                                                                                         vector<ASTPointer<Expression>>{make_shared<Literal>(empty_loc_, Token::Number, make_shared<ASTString>("0"))},
                                                                                                                         vector<ASTPointer<ASTString>>{}),
                                                                                               make_shared<Literal>(empty_loc_,
                                                                                                                    Token::StringLiteral,
                                                                                                                    make_shared<ASTString>(oracleTypeToString.at(oracle_query->type()))),
                                                                                               make_shared<Literal>(empty_loc_,
                                                                                                                    Token::StringLiteral,
                                                                                                                    make_shared<ASTString>("oraclize"))},
                                                                                           vector<ASTPointer<ASTString>>{}));

            false_stmts.push_back(log_oraclize);

#endif

            true_stmts.push_back(make_shared<ExpressionStatement>(empty_loc_, empty_doc_, call));
            false_stmts.push_back(stmt);

            auto if_stmt = make_shared<IfStatement>(empty_loc_,
                                                    empty_doc_,
                                                    cond,
                                                    make_shared<Block>(empty_loc_,
                                                                       empty_doc_,
                                                                       true_stmts),
                                                    make_shared<Block>(empty_loc_,
                                                                       empty_doc_,
                                                                       false_stmts));

            return if_stmt;
        }

        return stmt; // No decoration
    }
    default:
    {
        return stmt; // No decoration
    }
    }
}