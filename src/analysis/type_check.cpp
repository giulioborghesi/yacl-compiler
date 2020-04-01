#include <cool/analysis/type_check.h>
#include <cool/core/context.h>
#include <cool/ir/expr.h>

#include <iostream>

namespace cool {

Status TypeCheckPass::visit(Context *context, BinaryExprNode *node) {
  const auto *registry = context->classRegistry();
  const auto intTypeID = registry->typeID("Int");

  /// Type-check left subexpression
  auto statusLhs = node->lhs()->visitNode(context, this);
  if (!statusLhs.isOk()) {
    return statusLhs;
  }

  /// Type-check right subexpression
  auto statusRhs = node->rhs()->visitNode(context, this);
  if (!statusRhs.isOk()) {
    return statusRhs;
  }

  /// Type-check binary expression
  const auto lhsTypeID = node->lhs()->type().typeID;
  const auto rhsTypeID = node->rhs()->type().typeID;
  if (lhsTypeID != intTypeID || rhsTypeID != intTypeID) {
    return GenericError("Error: operand is not an integer");
  }

  /// Set expression type and return
  node->setType(ExprType{.typeID = intTypeID, .isSelf = false});
  return Status::Ok();
}

Status TypeCheckPass::visit(Context *context, BlockExprNode *node) {
  for (auto &subNode : node->exprs()) {
    auto status = subNode->visitNode(context, this);
    if (!status.isOk()) {
      return status;
    }
  }

  /// Exprs must always contain at least one expression
  node->setType(node->exprs().back()->type());
  return Status::Ok();
}

Status TypeCheckPass::visit(Context *context, BooleanExprNode *node) {
  const auto *registry = context->classRegistry();
  const auto boolTypeID = registry->typeID("Bool");

  node->setType(ExprType{.typeID = boolTypeID, .isSelf = false});
  return Status::Ok();
}

Status TypeCheckPass::visit(Context *context, IfExprNode *node) {
  const auto *registry = context->classRegistry();
  const auto boolTypeID = registry->typeID("Bool");

  /// Type-check if-expression
  auto statusIfExpr = node->ifExpr()->visitNode(context, this);
  if (!statusIfExpr.isOk()) {
    return statusIfExpr;
  }

  /// Type-check then-expression
  auto statusThenExpr = node->thenExpr()->visitNode(context, this);
  if (!statusThenExpr.isOk()) {
    return statusThenExpr;
  }

  /// Type-check else-expression
  auto statusElseExpr = node->elseExpr()->visitNode(context, this);
  if (!statusElseExpr.isOk()) {
    return statusElseExpr;
  }

  /// If-expression type must be Bool
  if (node->ifExpr()->type().typeID != boolTypeID) {
    return GenericError("Error: if-expression type is not Bool");
  }

  /// Compute expression type and return
  const auto thenType = node->thenExpr()->type();
  const auto elseType = node->elseExpr()->type();
  node->setType(registry->leastCommonAncestor(thenType, elseType));
  return Status::Ok();
}

Status TypeCheckPass::visit(Context *context, LiteralExprNode<int32_t> *node) {
  const auto *registry = context->classRegistry();
  const auto intTypeID = registry->typeID("Int");
  node->setType(ExprType{.typeID = intTypeID, .isSelf = false});
  return Status::Ok();
}

Status TypeCheckPass::visit(Context *context,
                            LiteralExprNode<std::string> *node) {
  const auto *registry = context->classRegistry();
  const auto stringTypeID = registry->typeID("String");
  node->setType(ExprType{.typeID = stringTypeID, .isSelf = false});
  return Status::Ok();
}

Status TypeCheckPass::visit(Context *context, NewExprNode *node) {
  const auto *registry = context->classRegistry();

  /// SELF_TYPE needs a special treatment
  if (node->typeName() == "SELF_TYPE") {
    const auto typeID = registry->typeID(context->currentClassName());
    node->setType(ExprType{.typeID = typeID, .isSelf = true});
    return Status::Ok();
  }

  const auto typeID = registry->typeID(node->typeName());
  if (typeID == -1) {
    return GenericError("Error: undefined type in new expression");
  }
  node->setType(ExprType{.typeID = typeID, .isSelf = false});
  return Status::Ok();
}

Status TypeCheckPass::visit(Context *context, UnaryExprNode *node) {
  /// Type-check subexpression
  auto statusExpr = node->expr()->visitNode(context, this);
  if (!statusExpr.isOk()) {
    return statusExpr;
  }

  /// Assign type to expression or return an error message on error
  switch (node->opID()) {
  case UnaryOpID::IsVoid: {
    return visitUnaryOpIsVoid(context, node);
  }
  case UnaryOpID::Not:
  case UnaryOpID::Complement: {
    const std::string type = node->opID() == UnaryOpID::Not ? "Bool" : "Int";
    return visitUnaryOpNotComp(context, node, type);
  }
  case UnaryOpID::Parenthesis: {
    node->setType(node->expr()->type());
    return Status::Ok();
  }
  default: {
    return GenericError("Error: unary operation not supported");
  }
  }

  /// Unreachable, added to suppress compiler warnings
  return Status::Ok();
}

Status TypeCheckPass::visit(Context *context, WhileExprNode *node) {
  /// Type-check loop condition expression
  auto statusLoopCond = node->loopCond()->visitNode(context, this);
  if (!statusLoopCond.isOk()) {
    return statusLoopCond;
  }

  const auto *registry = context->classRegistry();
  const auto boolTypeID = registry->typeID("Bool");
  if (node->loopCond()->type().typeID != boolTypeID) {
    return GenericError("Error: while-cond expression is not Bool");
  }

  /// Type-check loop body expression
  auto statusLoopBody = node->loopBody()->visitNode(context, this);
  if (!statusLoopBody.isOk()) {
    return statusLoopBody;
  }

  const auto objectTypeID = registry->typeID("Object");
  node->setType(ExprType{.typeID = objectTypeID, .isSelf = false});
  return Status::Ok();
}

Status TypeCheckPass::visitUnaryOpIsVoid(Context *context,
                                         UnaryExprNode *node) {
  /// Assign Bool type to expression
  const auto boolTypeID = context->classRegistry()->typeID("Bool");
  node->setType(ExprType{.typeID = boolTypeID, .isSelf = false});
  return Status::Ok();
}

Status TypeCheckPass::visitUnaryOpNotComp(Context *context, UnaryExprNode *node,
                                          const std::string &expectedType) {
  const auto expectedTypeID = context->classRegistry()->typeID(expectedType);

  /// Subexpression type must be bool
  if (node->expr()->type().typeID != expectedTypeID) {
    return GenericError("Error: operand of not is of incorrect type");
  }

  node->setType(ExprType{.typeID = expectedTypeID, .isSelf = false});
  return Status::Ok();
}

} // namespace cool
