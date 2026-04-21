// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace naja::NL::detail {

struct InferredMemoryGuardDefaults {
    int kind {0};
    bool polarity {false};
    bool exprNull {false};
    bool caseExprNull {false};
    bool caseStmtNull {false};
    bool caseItemNull {false};
    bool hasSourceRange {false};
};

struct FindTimedStatementTestResult {
    bool foundTimed {false};
    bool reportedUnsupported {false};
    std::string unsupportedReason;
};

struct CollectDirectAssignmentsTestResult {
    bool success {false};
    size_t assignmentCount {0};
    std::string failureReason;
};

struct ForLoopStepExpressionTestResult {
    bool success {false};
    int64_t loopValue {0};
    std::string failureReason;
};

struct SourceExcerptTestOptions {
    std::string sourceText;
    std::optional<size_t> startOffset {0};
    std::optional<size_t> endOffset {1};
    size_t maxLength {160};
    bool preserveRawBufferSize {false};
    bool useAlternateEndBuffer {false};
};

std::optional<std::string> testSVConstructorGetSourceExcerpt(
  const SourceExcerptTestOptions& options);

std::string testSVConstructorFormatReasonWithSourceExcerptNoRange(
  const std::string& reason);

bool testSVConstructorTryPowerInt64(
  int64_t base,
  int64_t exponent,
  int64_t& value);

InferredMemoryGuardDefaults testSVConstructorDefaultInferredMemoryGuard();

std::optional<std::vector<int32_t>> testSVConstructorCollectIndexedRangeElementIndices(
  int32_t startIndex,
  int32_t sliceWidth,
  bool indexedDown);

std::vector<bool> testSVConstructorEncodeUnsignedProductBits(
  uint64_t leftValue,
  uint64_t rightValue,
  size_t targetWidth);

std::optional<size_t> testSVConstructorResolveWildcardPatternWidthFallback(
  const std::optional<size_t>& directWidth,
  bool canonicalIntegral,
  int32_t bitWidth);

std::optional<std::string> testSVConstructorResolveWildcardCaseItemPatternFromAssignRhs(
  const std::string& sourceText,
  size_t targetWidth,
  bool signExtend);

std::optional<std::string> testSVConstructorResolveUnknownLiteralBitsAsZeroFromAssignRhs(
  const std::string& sourceText,
  size_t targetWidth);

std::optional<std::string> testSVConstructorResolveConstantExpressionBitsFromAssignRhs(
  const std::string& sourceText,
  size_t targetWidth);

std::vector<bool> testSVConstructorEncodeSignedInt64ConstantBits(
  int64_t value,
  size_t targetWidth);

std::optional<FindTimedStatementTestResult> testSVConstructorFindTimedStatementFromProceduralBlock(
  const std::string& sourceText);

std::optional<CollectDirectAssignmentsTestResult>
testSVConstructorCollectDirectAssignmentsFromProceduralBlock(
  const std::string& sourceText);

std::optional<ForLoopStepExpressionTestResult>
testSVConstructorApplyForLoopStepExpressionFromForLoop(
  const std::string& sourceText,
  int64_t initialLoopValue);

std::optional<ForLoopStepExpressionTestResult>
testSVConstructorApplyForLoopStepExpressionFromProceduralStatement(
  const std::string& sourceText,
  int64_t initialLoopValue);

std::optional<bool> testSVConstructorSameExpressionStructureFromContinuousAssignRhsPair(
  const std::string& sourceText);

std::string testSVConstructorFormatAssignmentLHSResolutionFailureReason(
  bool hasLhsNet,
  const std::string& lhsDescription,
  const std::string& failureReason);

std::string testSVConstructorFormatSequentialConcatLeafFailureReason(
  const std::string& lhsDescription,
  const std::string& leafFailureReason);

std::string testSVConstructorFormatDescribedFailure(
  const std::string& prefix,
  const std::string& description);

std::string testSVConstructorFormatQuotedDescriptionFailure(
  const std::string& prefix,
  const std::string& description);

}  // namespace naja::NL::detail
