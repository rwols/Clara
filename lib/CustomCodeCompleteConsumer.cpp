#include "CustomCodeCompleteConsumer.hpp"
#include <llvm/Support/raw_os_ostream.h>

CustomCodeCompleteConsumer::CustomCodeCompleteConsumer(const clang::CodeCompleteOptions& options)
: clang::CodeCompleteConsumer(options, false)
, CCTUInfo(new clang::GlobalCodeCompletionAllocator)
{
	/* empty */
}

void CustomCodeCompleteConsumer::ProcessCodeCompleteResults(
	clang::Sema &sema, 
	clang::CodeCompletionContext context,
	clang::CodeCompletionResult* results,
	unsigned numResults)
{
	std::stable_sort(results, results + numResults, [](const auto& lhs, const auto& rhs) 
	{
		return lhs.Priority > rhs.Priority;
	});

	for (unsigned i = 0; i < numResults; ++i) 
	{
		mResultList.append(ProcessCodeCompleteResult(sema, context, results[i]));
	}
}

void CustomCodeCompleteConsumer::ProcessOverloadCandidates(
	clang::Sema &sema, 
	unsigned currentArg,
	clang::CodeCompleteConsumer::OverloadCandidate* candidates,
	unsigned numCandidates) 
{
	/* empty */
}

boost::python::list CustomCodeCompleteConsumer::ProcessCodeCompleteResult(
	clang::Sema& sema,
	clang::CodeCompletionContext context,
	clang::CodeCompletionResult& result)
{
	using namespace boost;
	using namespace clang;
	using std::cout;


	python::list pair;
	std::string first, second, informative;
	unsigned argCount = 0;

	switch (result.Kind) 
	{
		case CodeCompletionResult::RK_Declaration:
		{
			auto completion = result.CreateCodeCompletionString(sema, context, getAllocator(), CCTUInfo, includeBriefComments());
			if (completion == nullptr)
			{
				second = result.Declaration->getNameAsString();
				first = second + "\tDeclaration";
			}
			else
			{
				ProcessCodeCompleteString(*completion, argCount, first, second, informative);
				// auto comment = completion->getBriefComment();
				// if (comment != nullptr)
				// {
				// 	first.append("\t");
				// 	first.append(comment);
				// }
			}
			break;
		}

		case CodeCompletionResult::RK_Keyword:
			second = result.Keyword;
			first = second + "\tKeyword";
			break;

		case CodeCompletionResult::RK_Macro: 
		{
			auto completion = result.CreateCodeCompletionString(sema, context, getAllocator(), CCTUInfo, includeBriefComments());
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
			second = result.Pattern->getAsString();
			first = second + "\tPattern";
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

	pair.append(first);
	pair.append(second);
	return pair;
}

void CustomCodeCompleteConsumer::ProcessCodeCompleteString(
	const clang::CodeCompletionString& ccs, 
	unsigned& argCount,
	std::string& first, 
	std::string& second,
	std::string& informative) const
{
	using namespace clang;

	std::string onTheRight;

	for (const auto& chunk : ccs)
	{
		switch (chunk.Kind)
		{
			case CodeCompletionString::CK_TypedText:
				// The piece of text that the user is expected to type to match the code-completion string,
				// typically a keyword or the name of a declarator or macro.
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_Text:
				// A piece of text that should be placed in the buffer,
				// e.g., parentheses or a comma in a function call.
				first += chunk.Text;
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
				first += chunk.Text;
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
				first += chunk.Text;
				first += " ";
				break;
			case CodeCompletionString::CK_CurrentParameter:
				// A piece of text that describes the parameter that corresponds to 
				// the code-completion location within a function call, message send, 
				// macro invocation, etc.
				++argCount;
				first += chunk.Text;
				second += "$";
				second += std::to_string(argCount);
				// second += "${";
				// second += std::to_string(argCount);
				// second += ":";
				// second += chunk.Text;
				// second += "}";
				break;
			case CodeCompletionString::CK_LeftParen:
				// A left parenthesis ('(').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_RightParen:
				// A right parenthesis (')').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_LeftBracket:
				// A left bracket ('[').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_RightBracket:
				// A right bracket (']').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_LeftBrace:
				// A left brace ('{').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_RightBrace:
				// A right brace ('}').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_LeftAngle:
				// A left angle bracket ('<').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_RightAngle:
				// A right angle bracket ('>').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_Comma:
				// A comma separator (',').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_Colon:
				// A colon (':').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_SemiColon:
				// A semicolon (';').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_Equal:
				// An '=' sign.
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_HorizontalSpace:
				// Horizontal whitespace (' ').
				first += chunk.Text;
				second += chunk.Text;
				break;
			case CodeCompletionString::CK_VerticalSpace:
				// Vertical whitespace ('\n' or '\r\n', depending on the platform).
				first += chunk.Text;
				second += chunk.Text;
				break;
			default:
				break;
		}
		// second += " ";
	}
}

clang::CodeCompletionAllocator& CustomCodeCompleteConsumer::getAllocator() 
{
	return CCTUInfo.getAllocator();
}

clang::CodeCompletionTUInfo& CustomCodeCompleteConsumer::getCodeCompletionTUInfo()
{
	return CCTUInfo;
}