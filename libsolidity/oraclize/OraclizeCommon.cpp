/** 
 *  @file    OraclizeCommon.cpp
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Commons. 
 *
 *  @section DESCRIPTION
 *  
 *  Commons for oraclizing contracts.
 */

#include <libsolidity/oraclize/OraclizeCommon.h>

using namespace std;
using namespace dev;
using namespace dev::solidity;

namespace dev
{
namespace solidity
{

const std::map<OracleType, string> oracleTypeToURL{
    {OracleType::Data, ""},
    {OracleType::Sort, "https://oraclize-solidity.herokuapp.com/sort/"},
    {OracleType::Sqrt, "https://oraclize-solidity.herokuapp.com/sqrt/"},
    {OracleType::Min, "https://oraclize-solidity.herokuapp.com/min/"},
    {OracleType::ThreeSum, "https://oraclize-solidity.herokuapp.com/3sum/"},
    {OracleType::KP, "https://oraclize-solidity.herokuapp.com/kp/"},
    {OracleType::APSP, "https://oraclize-solidity.herokuapp.com/apsp/"},
    {OracleType::KDS, "https://oraclize-solidity.herokuapp.com/kds/"}};

const std::map<string, OracleType> stringToOracleType{
    {"data", OracleType::Data},
    {"sort", OracleType::Sort},
    {"sqrt", OracleType::Sqrt},
    {"min", OracleType::Min},
    {"3sum", OracleType::ThreeSum},
    {"kp", OracleType::KP},
    {"apsp", OracleType::APSP},
    {"kds", OracleType::KDS}};

const std::map<OracleType, string> oracleTypeToString{
    {OracleType::Data, "data"},
    {OracleType::Sort, "sort"},
    {OracleType::Sqrt, "sqrt"},
    {OracleType::Min, "min"},
    {OracleType::ThreeSum, "3sum"},
    {OracleType::KP, "kp"},
    {OracleType::APSP, "apsp"},
    {OracleType::KDS, "kds"}};

string StringToLower(string s)
{
    for (string::iterator it = s.begin(); it != s.end(); ++it)
    {
        *it = tolower(*it);
    }

    return s;
}

bool StringCompareI(string s1, string s2)
{
    return StringToLower(s1).compare(StringToLower(s2)) == 0;
}

const string GetOracleIdentifier()
{
    return string("oracleQuery");
}

OracleType OracleQuery::type()
{
    return type_;
}

ASTPointer<Identifier> OracleQuery::callback()
{
    return callback_;
}

FunctionDefinition *OracleQuery::container_func()
{
    return container_func_;
}

std::string OracleQuery::env_name()
{
    return env_name_;
}

void OracleQuery::set_env_name(string env_name)
{
    env_name_ = env_name;
}

std::string OracleQuery::var_name()
{
    return var_name_;
}

void OracleQuery::set_var_name(string var_name)
{
    var_name_ = var_name;
}

string OracleQuery::ToString()
{
    string s;

    s += string(INDENT, ' ');
    s += "Type: ";
    s += oracleTypeToString.at(type()) + "\n";
    s += string(INDENT, ' ');
    s += "Callback: " + (callback() ? callback()->name() : "") + "\n";
    s += string(INDENT, ' ');
    s += "Function: " + (container_func() ? container_func()->name() : "") + "\n";
    s += string(INDENT, ' ');
    s += "Environment: " + env_name() + "\n";
    s += string(INDENT, ' ');
    s += "Variable: " + var_name() + "\n";

    return s;
}

std::vector<std::string> DataQuery::urls()
{
    return urls_;
}

string DataQuery::ToString()
{
    string s;

    s += OracleQuery::ToString();
    s += string(INDENT, ' ');
    s += "URLs: ";

    for (auto url : urls())
    {
        s += url + " ";
    }

    s += "\n";

    return s;
}

size_t DataQuery::QuerySize()
{
    return urls_.size();
}

UrlQuery::~UrlQuery() {}

string UrlQuery::url()
{
    return url_;
}

string UrlQuery::ToString()
{
    string s;

    s += OracleQuery::ToString();
    s += string(INDENT, ' ');
    s += "URL: " + url() + "\n";

    return s;
}

size_t UrlQuery::QuerySize()
{
    return 1;
}

SwitchableQuery::~SwitchableQuery() {}

ASTPointer<Identifier> SwitchableQuery::switch_func()
{
    return switch_func_;
}

string SwitchableQuery::ToString()
{
    string s;

    s += UrlQuery::ToString();
    s += string(INDENT, ' ');
    s += "Switch: " + (switch_func() ? switch_func()->name() : "") + "\n";

    return s;
}

ExpressionQuery::~ExpressionQuery() {}

ASTPointer<Expression> ExpressionQuery::expression()
{
    return expression_;
}

string ExpressionQuery::ToString()
{
    string s;

    s += SwitchableQuery::ToString();
    s += string(INDENT, ' ');
    s += "Expression: ";

    if (dynamic_cast<const Literal *>(expression().get()))
    {
        s += dynamic_cast<const Literal *>(expression().get())->value();
    }
    else if (dynamic_cast<const Identifier *>(expression().get()))
    {
        s += dynamic_cast<const Identifier *>(expression().get())->name();
    }

    s += "\n";

    return s;
}

VerifierQuery::~VerifierQuery() {}

bool VerifierQuery::verify()
{
    return verify_;
}

string VerifierQuery::ToString()
{
    string s;

    s += ExpressionQuery::ToString();
    s += string(INDENT, ' ');
    s += "Verify: " + (verify() ? string("true") : string("false")) + "\n";

    return s;
}

ASTPointer<Expression> ThreeSumQuery::sum()
{
    return sum_;
}

string ThreeSumQuery::ToString()
{
    string s;

    s += VerifierQuery::ToString();
    s += string(INDENT, ' ');
    s += "Sum: ";

    if (dynamic_cast<const Literal *>(sum().get()))
    {
        s += dynamic_cast<const Literal *>(sum().get())->value();
    }
    else if (dynamic_cast<const Identifier *>(sum().get()))
    {
        s += dynamic_cast<const Identifier *>(sum().get())->name();
    }

    s += "\n";

    return s;
}

ASTPointer<Expression> KPQuery::path_len()
{
    return path_len_;
}

ASTPointer<Expression> KPQuery::max_path_weight()
{
    return max_path_weight_;
}

string KPQuery::ToString()
{
    string s;

    s += VerifierQuery::ToString();
    s += string(INDENT, ' ');
    s += "Path Length: ";

    if (dynamic_cast<const Literal *>(path_len().get()))
    {
        s += dynamic_cast<const Literal *>(path_len().get())->value();
    }
    else if (dynamic_cast<const Identifier *>(path_len().get()))
    {
        s += dynamic_cast<const Identifier *>(path_len().get())->name();
    }

    s += "\n";

    s += string(INDENT, ' ');
    s += "Maximum Weight: ";

    if (dynamic_cast<const Literal *>(max_path_weight().get()))
    {
        s += dynamic_cast<const Literal *>(max_path_weight().get())->value();
    }
    else if (dynamic_cast<const Identifier *>(max_path_weight().get()))
    {
        s += dynamic_cast<const Identifier *>(max_path_weight().get())->name();
    }

    s += "\n";

    return s;
}

ASTPointer<Expression> KDSQuery::max_size()
{
    return max_size_;
}

string KDSQuery::ToString()
{
    string s;

    s += VerifierQuery::ToString();
    s += string(INDENT, ' ');
    s += "Maximum Size: ";

    if (dynamic_cast<const Literal *>(max_size().get()))
    {
        s += dynamic_cast<const Literal *>(max_size().get())->value();
    }
    else if (dynamic_cast<const Identifier *>(max_size().get()))
    {
        s += dynamic_cast<const Identifier *>(max_size().get())->name();
    }

    s += "\n";

    return s;
}
}
}