/** 
 *  @file    OraclizePass.h
 *  @author  Nicolas Lykke Iversen (nlykkei)
 *  @date    1/1/2018  
 *  @version 1.0 
 *  
 *  @brief Oraclize, Oraclize compiler pass.
 *
 *  @section DESCRIPTION
 *  
 *  Compiler pass for oraclizing contracts by source-to-source transformations.
 *  
 */

#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include <libsolidity/analysis/TypeChecker.h>
#include <libsolidity/ast/Types.h>
#include <libsolidity/ast/ASTAnnotations.h>
#include <libsolidity/ast/ASTForward.h>
#include <libsolidity/ast/ASTVisitor.h>

#include <libsolidity/oraclize/OraclizeCommon.h>
#include <libsolidity/oraclize/OraclizeTransform.h>

namespace dev
{
namespace solidity
{

/**
 * Class for the compiler pass to oraclize contracts by means of AST transformations.
 */
class OraclizePass : private ASTConstVisitor
{
public:
  explicit OraclizePass(ErrorReporter &error_reporter, uint gas_limit = 0, uint gas_price = 0)
      : error_reporter_(error_reporter), gas_limit_(gas_limit), gas_price_(gas_price) {}
  bool analyze(const SourceUnit &source_unit);

private:
  virtual bool visit(const ContractDefinition &contract) override;
  virtual void endVisit(const ContractDefinition &contract) override;

  virtual bool visit(const FunctionDefinition &function) override;
  virtual void endVisit(const FunctionDefinition &function) override;

  virtual bool visit(const VariableDeclaration &variable) override;
  virtual bool visit(const Identifier &identifier) override;

  virtual bool visit(const FunctionCall &call) override;

  /** 
   *   @brief  Parses identifier at given position.   
   *  
	 *   @param  call_args is a vector of arguments
	 *   @param  pos is the position  
   *   @return shared_ptr<Identifier>
   */
  ASTPointer<Identifier> ParseIdentifier(const std::vector<ASTPointer<const Expression>> &call_args, const size_t pos);

  /** 
   *   @brief  Parses identifier or number at given position.   
   *  
	 *   @param  call_args is a vector of arguments
	 *   @param  pos is the position  
   *   @return shared_ptr<Expression>
   */
  ASTPointer<Expression> ParseIdentifierOrNumber(const std::vector<ASTPointer<const Expression>> &call_args, const size_t pos);

  /** 
   *   @brief  Parses bool at given position.   
   *  
	 *   @param  call_args is a vector of arguments
	 *   @param  pos is the position  
   *   @return bool
   */
  bool ParseBool(const std::vector<ASTPointer<const Expression>> &call_args, const size_t pos);

  /** 
   *   @brief  Parses string at given position.   
   *  
	 *   @param  call_args is a vector of arguments
   * 	 @param  pos is the position  
   *   @return string
   */
  std::string ParseString(const std::vector<ASTPointer<const Expression>> &call_args, const size_t pos);

  ErrorReporter &error_reporter_;                            ///< Error reporter
  const uint gas_limit_;                                     ///< User-specified gas limit for Oraclize queries
  const uint gas_price_;                                     ///< User-specified gas price for Oraclize queries
  std::shared_ptr<OraclizeTransform> oraclize_transform_;    ///< Oraclize transformer (source-to-source)
  FunctionDefinition const *current_func_ = nullptr;         ///< Current function being visited
  std::vector<std::shared_ptr<OracleQuery>> oracle_queries_; ///< List of Oraclize queries
};
}
}
