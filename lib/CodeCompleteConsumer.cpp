#include "CodeCompleteConsumer.hpp"
#include "PyBind11.hpp"

#define DEBUG_PRINT llvm::errs() << __FILE__ << ':' << __LINE__ << '\n'

namespace Clara {

CodeCompleteConsumer::CodeCompleteConsumer(const clang::CodeCompleteOptions& options, 
	clang::IntrusiveRefCntPtr<clang::FileManager> fileManager, 
	std::string filename, int row, int column)
: clang::CodeCompleteConsumer(options, false)
, DiagOpts(new clang::DiagnosticOptions)
, Diag(new clang::DiagnosticsEngine(clang::IntrusiveRefCntPtr<clang::DiagnosticIDs>(new clang::DiagnosticIDs), &*DiagOpts))
, FileMgr(fileManager)
, SourceMgr(new clang::SourceManager(*Diag, *FileMgr))
, mFilename(std::move(filename))
, mRow(row)
, mColumn(column)
, mCCTUInfo(new clang::GlobalCodeCompletionAllocator)
{
	/* empty */
}

void CodeCompleteConsumer::ProcessCodeCompleteResults(
	clang::Sema &sema, 
	clang::CodeCompletionContext context,
	clang::CodeCompletionResult* results,
	unsigned numResults)
{
	// Clear the list, from other runs.
	mResultList.clear();
    mResultList.reserve(numResults);

	std::stable_sort(results, results + numResults, [](const auto& lhs, const auto& rhs) 
	{
		return lhs.Priority < rhs.Priority;
	});

	for (unsigned i = 0; i < numResults; ++i) 
	{
        if (results[i].Availability == CXAvailability_NotAvailable ||
            results[i].Availability == CXAvailability_NotAccessible)
        {
            continue;
        }
        else
        {
            mResultList.emplace_back(ProcessCodeCompleteResult(sema, context, results[i]));
        }
	}
}

void CodeCompleteConsumer::ProcessOverloadCandidates(
	clang::Sema &sema, 
	unsigned currentArg,
	clang::CodeCompleteConsumer::OverloadCandidate* candidates,
	unsigned numCandidates) 
{
	/* empty */
}

std::pair<std::string, std::string> CodeCompleteConsumer::ProcessCodeCompleteResult(
	clang::Sema& sema,
	clang::CodeCompletionContext context,
	clang::CodeCompletionResult& result)
{
	using namespace clang;

	std::string first, second, informative;
	unsigned argCount = 0;

	switch (result.Kind) 
	{
		case CodeCompletionResult::RK_Declaration:
		{
			auto completion = result.CreateCodeCompletionString(sema, context, getAllocator(), mCCTUInfo, includeBriefComments());
			if (completion == nullptr)
			{
				second = result.Declaration->getNameAsString();
				first = second + "\tDeclaration";
			}
			else
			{
				ProcessCodeCompleteString(*completion, argCount, first, second, informative);
			}
			break;
		}

		case CodeCompletionResult::RK_Keyword:
			second = result.Keyword;
			first = second + "\tKeyword";
			break;

		case CodeCompletionResult::RK_Macro: 
		{
			auto completion = result.CreateCodeCompletionString(sema, context, getAllocator(), mCCTUInfo, includeBriefComments());
			if (completion == nullptr)
			{
				second = result.Macro->getNameStart();
				first = second + "\tMacro";
			}
			else
			{
				ProcessCodeCompleteString(*completion, argCount, first, second, informative);
			}
			break;
		}

		case CodeCompletionResult::RK_Pattern: 
		{
			auto completion = result.CreateCodeCompletionString(sema, context, getAllocator(), mCCTUInfo, includeBriefComments());
			if (completion == nullptr)
			{
				second = result.Macro->getNameStart();
				first = second + "\tPattern";
			}
			else
			{
				ProcessCodeCompleteString(*completion, argCount, first, second, informative);
			}
			break;
		}
	}

	if (argCount > 0)
	{
		second += "$0";
	}

	if (!informative.empty())
	{
		first += "\t";
		first += informative;
	}
    
    return std::make_pair<std::string, std::string>(std::move(first), std::move(second));
}

void CodeCompleteConsumer::ProcessCodeCompleteString(
	const clang::CodeCompletionString& ccs, 
	unsigned& argCount,
	std::string& first, 
	std::string& second,
	std::string& informative) const
{
	using namespace clang;

	for (unsigned j = 0; j < ccs.getAnnotationCount(); ++j)
	{
		const char* annotation = ccs.getAnnotation(j);
		informative += annotation;
		if (j != ccs.getAnnotationCount() - 1) informative += ' ';
	}

	for (const auto& chunk : ccs)
	{
		switch (chunk.Kind)
		{
			case CodeCompletionString::CK_TypedText:
				// The piece of text that the user is expected to type to match the code-completion string,
				// typically a keyword or the name of a declarator or macro.
				first += chunk.Text;
				second += chunk.Text;
				informative += chunk.Text;
				break;
			case CodeCompletionString::CK_Text:
				// A piece of text that should be placed in the buffer,
				// e.g., parentheses or a comma in a function call.
				// first += chunk.Text;
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_Optional:
				// A code completion string that is entirely optional.
				// For example, an optional code completion string that
				// describes the default arguments in a function call.
				if (includeOptionalArguments)
				{
					ProcessCodeCompleteString(*chunk.Optional, argCount, first, second, informative);
				}
				break;
			case CodeCompletionString::CK_Placeholder:
				// A string that acts as a placeholder for, e.g., a function call argument.
				++argCount;
				informative += chunk.Text;
				second += "${";
				second += std::to_string(argCount);
				second += ":";
				second += chunk.Text;
				second += "}";
				break;
			case CodeCompletionString::CK_Informative:
				// A piece of text that describes something about the result
				// but should not be inserted into the buffer.
				informative += chunk.Text;
				break;
			case CodeCompletionString::CK_ResultType:
				// A piece of text that describes the type of an entity or, 
				// for functions and methods, the return type.
				informative += chunk.Text;
				informative += " ";
				break;
			case CodeCompletionString::CK_CurrentParameter:
				// A piece of text that describes the parameter that corresponds to 
				// the code-completion location within a function call, message send, 
				// macro invocation, etc.
				++argCount;
				informative += chunk.Text;
				second += "${";
				second += std::to_string(argCount);
				second += ":";
				second += chunk.Text;
				second += "}";
				break;
			case CodeCompletionString::CK_LeftParen:
				// A left parenthesis ('(').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_RightParen:
				// A right parenthesis (')').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_LeftBracket:
				// A left bracket ('[').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_RightBracket:
				// A right bracket (']').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_LeftBrace:
				// A left brace ('{').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_RightBrace:
				// A right brace ('}').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_LeftAngle:
				// A left angle bracket ('<').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_RightAngle:
				// A right angle bracket ('>').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_Comma:
				// A comma separator (',').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_Colon:
				// A colon (':').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_SemiColon:
				// A semicolon (';').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_Equal:
				// An '=' sign.
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_HorizontalSpace:
				// Horizontal whitespace (' ').
				informative += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_VerticalSpace:
				// Vertical whitespace ('\n' or '\r\n', depending on the platform).
				informative += chunk.Text;
				second += chunk.Text;
				break;
		}
	}

	if (ccs.getBriefComment() != nullptr)
	{
		informative += ": ";
		informative += ccs.getBriefComment();
	}
}

clang::CodeCompletionAllocator& CodeCompleteConsumer::getAllocator() 
{
	return mCCTUInfo.getAllocator();
}

clang::CodeCompletionTUInfo& CodeCompleteConsumer::getCodeCompletionTUInfo()
{
	return mCCTUInfo;
}

void CodeCompleteConsumer::moveResult(std::vector<std::pair<std::string, std::string>>& result)
{
	result = std::move(mResultList);
	mResultList.clear();
}

void CodeCompleteConsumer::clearResult()
{
	mResultList.clear();
}

} // namespace Clara
