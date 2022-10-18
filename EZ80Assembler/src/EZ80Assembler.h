#pragma once

#include <filesystem>
#include <vector>

namespace ez80
{
	enum AssemblerError_ : uint32_t
	{
		AssemblerError_None = 0,

		AssemblerError_MissingInputFile,
		AssemblerError_InvalidInputFileExtension,
		AssemblerError_OutputFileNameInvalid,
		AssemblerError_FailedToReadInputFile,
		AssemblerError_InvalidStringLiteral,
		AssemblerError_InvalidPreprocessorStatement,
		AssemblerError_MacroArgsMustStartWithDollarSign,
		AssemblerError_InvalidDotDirectiveOrInstructionParameters,
		AssemblerError_InvalidDotDirectiveParameters,
		AssemblerError_InvalidInstructionOpcodes,


		// At the very end of the error list. (approximately ordered in the order they can happen in)
		AssemblerError_AssemblyEmpty,
		AssemblerError_AssemblyTooLarge,
		AssemblerError_FailedToWriteOutputFile,
	};
	struct AssemblerError
	{
		using ID = std::underlying_type_t<AssemblerError_>;

		constexpr AssemblerError(ID id = 0, size_t lineNumber = 0) noexcept
			: id(id), lineNumber(lineNumber + 1) {}

		constexpr operator ID() const noexcept { return id; }

		ID id;
		size_t lineNumber;
	};

	enum AssemblerWarning_ : uint32_t
	{
		AssemblerWarning_AssemblyDoesntStartWithEF_7B,
		AssemblerWarning_NoAssemblyProduced,
		AssemblerWarning_OpcodeTrailingComma
	};
	struct AssemblerWarning
	{
		using ID = std::underlying_type_t<AssemblerWarning_>;

		constexpr AssemblerWarning(ID id = 0, size_t lineNumber = 0) noexcept
			: id(id), lineNumber(lineNumber + 1) {}

		constexpr operator ID() const noexcept { return id; }

		ID id = 0;
		size_t lineNumber = 0;
	};

	struct AssemblerResult
	{
		constexpr AssemblerResult() noexcept = default;
		constexpr AssemblerResult& Error(AssemblerError error) noexcept { this->error = error; return *this; }
		explicit constexpr operator bool() const noexcept { return error != AssemblerError_None; }

		// Only the first error is reported.
		AssemblerError error = AssemblerError_None;
		std::vector<AssemblerWarning> warnings;
	};

	struct AssemblerInfo
	{
		std::filesystem::path inputFilepath;
		std::filesystem::path outputFilepath;
		std::vector<std::filesystem::path> includeDirectories;
	};

	// Returns 0 on success, non-zero otherwise.
	AssemblerResult Assemble(const AssemblerInfo& info);
}
