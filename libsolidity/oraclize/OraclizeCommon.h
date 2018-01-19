/** 
 *  @file    OraclizeCommon.h
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

#pragma once

#include <string>
#include <map>

#include <libsolidity/ast/AST.h>

#define COMPILER_DEBUG ///< Enable/disable compiler debugging
#undef AST_DEBUG       ///< Enable/disable AST debugging
#define CONTRACT_DEBUG  ///< Enable/disable contract debugging

#undef UINT_256 ///< Enable/disable uint256 type

#define THREESUM_UINT_X 16 ///< Lightweight uintX type for "3sum"
#define KP_UINT_X 16 ///< Lightweight uintX type for "kp"
#define KDS_UINT_X 8 ///< Lightweight uintX type for "kds"

#define INDENT 4 ///< Identation for string representations

namespace dev
{
namespace solidity
{

/**
 * Enum for types of Oraclize queries.
 */
enum class OracleType
{
  Data,
  Sort,
  Sqrt,
  Min,
  ThreeSum,
  KP,
  APSP,
  KDS
};

extern const std::map<OracleType, std::string> oracleTypeToURL;    ///< Mapping from Oraclize query types to webservice URLs
extern const std::map<std::string, OracleType> stringToOracleType; ///< Mapping from strings to Oraclize query types
extern const std::map<OracleType, std::string> oracleTypeToString; ///< Mapping from  Oraclize query types to strings

/**
 * Exception for non-implemented features.
 */
class NotImplementedException : public std::logic_error
{
public:
  NotImplementedException() : std::logic_error("Feature not yet implemented."){};
  NotImplementedException(std::string what) : std::logic_error(what) {}
};

/**
 * Exception for invalid input.
 */
class OraclizeSolidityException : public std::logic_error
{
public:
  OraclizeSolidityException() : std::logic_error("OraclizeSolidity."){};
  OraclizeSolidityException(std::string what) : std::logic_error(what) {}
};

/** 
 *   @brief  Identifier for Oraclize queries (e.g., "oracleQuery").   
 *  
 *   @return string
 */
const std::string GetOracleIdentifier();

/** 
 *   @brief  Converts a string to lowercase.   
 *  
 *   @return string
 */
std::string StringToLower(std::string s);

/** 
 *   @brief  Case insensitive string comparison.   
 *  
 *   @return bool
 */
bool StringCompareI(std::string s1, std::string s2);

/**
 * Abstract base class for Oraclize queries.
 */
class OracleQuery
{
public:
  OracleQuery(OracleType type,
              ASTPointer<Identifier> callback,
              FunctionDefinition *container_func,
              std::string env_name = "",
              std::string var_name = "") : type_(type),
                                           callback_(callback),
                                           container_func_(container_func),
                                           env_name_(env_name),
                                           var_name_(var_name)
  {
  }

  /** 
   *   @brief  Converts OracleQuery to its string representation.   
   *  
   *   @return string representation as string
   */
  virtual std::string ToString();

  /** 
   *   @brief  Query size of the OracleQuery, i.e., the number of requests to perform.   
   *  
   *   @return query size as size_t
   */
  virtual size_t QuerySize() = 0;

  /** 
   *   @brief  Type of the OracleQuery.   
   *  
   *   @return type as OracleType
   */
  OracleType type();

  /** 
   *   @brief  Callback of the OracleQuery.   
   *  
   *   @return callback as shared_ptr<Identifier>
   */
  ASTPointer<Identifier> callback();

  /** 
   *   @brief  Function containing the the OracleQuery.   
   *  
   *   @return function as FunctionDefinition *
   */
  FunctionDefinition *container_func();

  /** 
   *   @brief  Name of the environment definition for the OracleQuery.   
   *  
   *   @return name as string
   */
  std::string env_name();

  /** 
   *   @brief  Set name of the environment definition for the OracleQuery.   
   *  
   *   @return void
   */
  void set_env_name(std::string env_name);

  /** 
   *   @brief  Name of the environment variable for the OracleQuery.   
   *  
   *   @return name as string
   */
  std::string var_name();

  /** 
   *   @brief  Set name of the environment variable for the OracleQuery.   
   *  
   *   @return void
   */
  void set_var_name(std::string var_name);

private:
  OracleType type_;                    ///< Type of Oraclize query
  ASTPointer<Identifier> callback_;    ///< Callback function to invoke with obtained results
  FunctionDefinition *container_func_; ///< Function containing the Oraclize query
  std::string env_name_;               ///< The name of the corresponding environment definition
  std::string var_name_;               ///< The name of the corresponding environment variable
};

/**
 * Class for "data" Oraclize queries.
 */
class DataQuery : public OracleQuery
{
public:
  DataQuery(OracleType type,
            std::vector<std::string> urls,
            ASTPointer<Identifier> callback,
            FunctionDefinition *container_func,
            std::string env_name = "",
            std::string var_name = "") : OracleQuery(type, callback, container_func, env_name, var_name),
                                         urls_(urls)
  {
  }

  std::string ToString() override;
  size_t QuerySize() override;

  /** 
   *   @brief  List of URLs for the DataQuery.   
   *  
   *   @return void
   */
  std::vector<std::string> urls();

private:
  std::vector<std::string> urls_; ///< The URLs to request data
};

/**
 * Abstract base class for URL Oraclize queries.
 */

class UrlQuery : public OracleQuery
{
public:
  UrlQuery(OracleType type,
           ASTPointer<Identifier> callback,
           std::string url,
           FunctionDefinition *container_func,
           std::string env_name = "",
           std::string var_name = "") : OracleQuery(type, callback, container_func, env_name, var_name),
                                        url_(url)
  {
  }
  virtual ~UrlQuery() = 0;

  std::string ToString() override;
  size_t QuerySize() override;

  /** 
   *   @brief  Request URL (might be empty in which case defaults to webservice).   
   *  
   *   @return URL as string
   */
  std::string url();

private:
  std::string url_; ///< Request URL
};

/**
 * Abstract base class for switchable Oraclize queries.
 */
class SwitchableQuery : public UrlQuery
{
public:
  SwitchableQuery(OracleType type,
                  ASTPointer<Identifier> callback,
                  std::string url,
                  ASTPointer<Identifier> switch_func,
                  FunctionDefinition *container_func,
                  std::string env_name = "",
                  std::string var_name = "") : UrlQuery(type, callback, url, container_func, env_name, var_name),
                                               switch_func_(switch_func)
  {
  }
  virtual ~SwitchableQuery() = 0;

  std::string ToString() override;

  /** 
   *   @brief  Switch function for the SwitchableQuery.   
   *  
   *   @return switch function as shared_ptr<Identifier>
   */
  ASTPointer<Identifier> switch_func();

private:
  ASTPointer<Identifier> switch_func_; ///< Switch function
};

/**
 * Abstract base class for Oraclize queries whose (partial) obtained from expression.
 */
class ExpressionQuery : public SwitchableQuery
{
public:
  ExpressionQuery(OracleType type,
                  ASTPointer<Expression> expression,
                  ASTPointer<Identifier> callback,
                  std::string url,
                  ASTPointer<Identifier> switch_func,
                  FunctionDefinition *container_func,
                  std::string env_name = "",
                  std::string var_name = "") : SwitchableQuery(type, callback, url, switch_func, container_func, env_name, var_name),
                                               expression_(expression)
  {
  }
  virtual ~ExpressionQuery() = 0;

  std::string ToString() override;

  /** 
   *   @brief  Expression for the ExpressionQuery.   
   *  
   *   @return expression as shared_ptr<Expression>
   */
  ASTPointer<Expression> expression();

private:
  ASTPointer<Expression> expression_; ///< The expression providing input
};

/**
 * Class for "min" Oraclize queries.
 */
class MinQuery : public ExpressionQuery
{
public:
  MinQuery(OracleType type,
           ASTPointer<Identifier> array_id,
           ASTPointer<Identifier> callback,
           std::string url,
           ASTPointer<Identifier> switch_func,
           FunctionDefinition *container_func,
           std::string env_name = "",
           std::string var_name = "") : ExpressionQuery(type, array_id, callback, url, switch_func, container_func, env_name, var_name)
  {
  }
};

/**
 * Class for "apsp" (all-pairs shortest path) Oraclize queries.
 */
class APSPQuery : public ExpressionQuery
{
public:
  APSPQuery(OracleType type,
            ASTPointer<Identifier> array_id,
            ASTPointer<Identifier> callback,
            std::string url,
            ASTPointer<Identifier> switch_func,
            FunctionDefinition *container_func,
            std::string env_name = "",
            std::string var_name = "") : ExpressionQuery(type, array_id, callback, url, switch_func, container_func, env_name, var_name)
  {
  }
};

/**
 * Abstract base class for verifiable Oraclize queries.
 */
class VerifierQuery : public ExpressionQuery
{
public:
  VerifierQuery(OracleType type,
                ASTPointer<Expression> expression,
                ASTPointer<Identifier> callback,
                bool verify,
                std::string url,
                ASTPointer<Identifier> switch_func,
                FunctionDefinition *container_func,
                std::string env_name = "",
                std::string var_name = "") : ExpressionQuery(type, expression, callback, url, switch_func, container_func, env_name, var_name),
                                             verify_(verify)
  {
  }
  virtual ~VerifierQuery() = 0;

  std::string ToString() override;

  /** 
   *   @brief  Whether or not the obtained result should be verified.   
   *  
   *   @return bool
   */
  bool verify();

private:
  bool verify_; ///< Result verification
};

/**
 * Class for "sort" Oraclize queries.
 */
class SortQuery : public VerifierQuery
{
public:
  SortQuery(OracleType type,
            ASTPointer<Identifier> array_id,
            ASTPointer<Identifier> callback,
            bool verify,
            std::string url,
            ASTPointer<Identifier> switch_func,
            FunctionDefinition *container_func,
            std::string env_name = "",
            std::string var_name = "") : VerifierQuery(type, array_id, callback, verify, url, switch_func, container_func, env_name, var_name)
  {
  }
};

/**
 * Class for "sqrt" Oraclize queries.
 */
class SqrtQuery : public VerifierQuery
{
public:
  SqrtQuery(OracleType type,
            ASTPointer<Expression> number,
            ASTPointer<Identifier> callback,
            bool verify,
            std::string url,
            ASTPointer<Identifier> switch_func,
            FunctionDefinition *container_func,
            std::string env_name = "",
            std::string var_name = "") : VerifierQuery(type, number, callback, verify, url, switch_func, container_func, env_name, var_name)
  {
  }
};

/**
 * Class for "threeSum" Oraclize queries.
 */
class ThreeSumQuery : public VerifierQuery
{
public:
  ThreeSumQuery(OracleType type,
                ASTPointer<Identifier> array_id,
                ASTPointer<Expression> sum,
                ASTPointer<Identifier> callback,
                bool verify,
                std::string url,
                ASTPointer<Identifier> switch_func,
                FunctionDefinition *container_func,
                std::string env_name = "",
                std::string var_name = "") : VerifierQuery(type, array_id, callback, verify, url, switch_func, container_func, env_name, var_name),
                                             sum_(sum)
  {
  }

  std::string ToString() override;

  /** 
   *   @brief  Sum.   
   *  
   *   @return sum as shared_ptr<Expression>
   */
  ASTPointer<Expression> sum();

private:
  ASTPointer<Expression> sum_;
};

/**
 * Class for "kp" (path of given criteria) Oraclize queries.
 */
class KPQuery : public VerifierQuery
{
public:
  KPQuery(OracleType type,
          ASTPointer<Identifier> array_id,
          ASTPointer<Expression> path_len,
          ASTPointer<Expression> max_path_weight,
          ASTPointer<Identifier> callback,
          bool verify,
          std::string url,
          ASTPointer<Identifier> switch_func,
          FunctionDefinition *container_func,
          std::string env_name = "",
          std::string var_name = "") : VerifierQuery(type, array_id, callback, verify, url, switch_func, container_func, env_name, var_name),
                                       path_len_(path_len), max_path_weight_(max_path_weight)
  {
  }

  std::string ToString() override;

  /** 
   *   @brief  Path length.   
   *  
   *   @return path length as shared_ptr<Expression>
   */
  ASTPointer<Expression> path_len();

  /** 
   *   @brief  Maximum path weight.   
   *  
   *   @return maximum path weight as shared_ptr<Expression>
   */
  ASTPointer<Expression> max_path_weight();

private:
  ASTPointer<Expression> path_len_;        ///< Path length (indentifier or literal)
  ASTPointer<Expression> max_path_weight_; ///< Maximum path weight (identifier or literal)
};

/**
 * Class for "kds" (dominating set of given criteria) Oraclize queries.
 */
class KDSQuery : public VerifierQuery
{
public:
  KDSQuery(OracleType type,
           ASTPointer<Identifier> array_id,
           ASTPointer<Expression> max_size,
           ASTPointer<Identifier> callback,
           bool verify,
           std::string url,
           ASTPointer<Identifier> switch_func,
           FunctionDefinition *container_func,
           std::string env_name = "",
           std::string var_name = "") : VerifierQuery(type, array_id, callback, verify, url, switch_func, container_func, env_name, var_name),
                                        max_size_(max_size)
  {
  }

  std::string ToString() override;

  /** 
   *   @brief  Maximum size.   
   *  
   *   @return maximum size as shared_ptr<Expression>
   */
  ASTPointer<Expression> max_size();

private:
  ASTPointer<Expression> max_size_; ///< Maximum size (indentifier or literal)
};
}
}