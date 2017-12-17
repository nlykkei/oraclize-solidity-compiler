/** 
 *  @file    OraclizeTransform.h
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

#pragma once

#include <string>
#include <memory>

#include <libsolidity/ast/AST.h>

#include <libsolidity/oraclize/OraclizeCommon.h>

namespace dev
{
namespace solidity
{
/**
 * Class for oraclizing contracts by means of source-to-source transformations performed on the AST.
 */
class OraclizeTransform
{
public:
  OraclizeTransform(SourceUnit *source_unit, uint gas_limit = 0, uint gas_price = 0)
      : source_unit_(source_unit),
        gas_limit_(gas_limit),
        gas_price_(gas_price),
        empty_doc_(std::make_shared<ASTString>("")),
        empty_loc_(SourceLocation(0, 0, std::make_shared<ASTString>(source_unit_->annotation().path)))
  {
  }

  /** 
   *   @brief  Inserts environment definitions and variable into given contract containing Oraclize queries  
   *  
   *   @param  contract is the ContractDefinition
   *   @param  oracle_queries is the Oraclize queries
   *   @return success as bool
   */
  bool OraclizeEnvironment(const ContractDefinition &contract, const std::vector<std::shared_ptr<OracleQuery>> &oracle_queries);

  /** 
   *   @brief  Inserts Oraclize event definition into given contract: OraclizeEvent(bytes32, string, string)   
   *  
   *   @param  contract is the ContractDefinition
   *   @return success as bool
   */
  bool OraclizeEvent(const ContractDefinition &contract);

  /** 
   *   @brief  Modifies the container functions of Oraclize queries
   *  
   *   @param  oracle_queries is the Oraclize queries
   *   @return success as bool
   */
  bool OraclizeContainer(const std::vector<std::shared_ptr<OracleQuery>> &oracle_queries);

  /** 
   *   @brief  Inserts Oraclize __callback() into given contract containing Oraclize queries
   *  
   *   @param  contract is the ContractDefinition
   *   @param  oracle_queries is the Oraclize queries
   *   @return success as bool 
   */
  bool OraclizeCallback(const ContractDefinition &contract, const std::vector<std::shared_ptr<OracleQuery>> &oracle_queries);

private:
  /** 
   *   @brief  Creates environment definition for given Oraclize query   
   *  
   *   @param  name is the name of the environment
   *   @param  oracle_query is the Oraclize query
   *   @return new environment as shared_ptr<StructDefinition>
   */
  ASTPointer<StructDefinition> EnvironmentDef(const ASTPointer<ASTString> &name, std::shared_ptr<OracleQuery> oracle_query);

  /** 
   *   @brief  Initializes environment for given Oraclize query   
   *  
   *   @param  oracle_query is the Oraclize query
   *   @return environment initialization as shared_ptr<Assignment>
   */
  ASTPointer<Assignment> EnvironmentInit(std::shared_ptr<OracleQuery> oracle_query);

  /** 
   *   @brief  Calls oraclize_query() for given Oraclize query   
   *  
   *   @param  oracle_query is the Oraclize query
   *   @return call and URL as pair of shared_ptr<FunctionCall> and shared_ptr<Expression>
   */
  std::pair<ASTPointer<FunctionCall>, ASTPointer<Expression>> OraclizeQuery(std::shared_ptr<OracleQuery> oracle_query);

  /** 
   *   @brief  Decorates statement with dynamic switching for given Oraclize query   
   *  
   *   @param  stmt is the statement
   *   @param  oracle_query is the Oraclize query
   *   @return decorated statement as shared_ptr<Statement>
   */
  ASTPointer<Statement> DynamicSwitching(ASTPointer<Statement> stmt, std::shared_ptr<OracleQuery> oracle_query);

  const SourceUnit *source_unit_;         ///< Current source unit being visited
  const uint gas_limit_;                  ///< User-specified gas limit for Oraclize queries
  const uint gas_price_;                  ///< User-specified gas price for Oraclize queries
  const ASTPointer<ASTString> empty_doc_; ///< Empty documentation
  const SourceLocation empty_loc_;        ///< Empty location
};
}
}