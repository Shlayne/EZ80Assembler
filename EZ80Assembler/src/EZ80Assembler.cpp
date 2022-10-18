#include "EZ80Assembler.h"
#include "AssemblerStringUtil.h"
#include "Debug.h"
#include <sstream>
#include <fstream>

namespace ez80
{
	struct TokenizedLine
	{
		using Iterator = std::vector<std::string_view>::iterator;

		constexpr TokenizedLine(std::vector<std::string_view>& tokens, size_t start, size_t tokenCount, size_t number) noexcept
			: tokenCount(tokenCount), number(number), beginIt(tokens.begin() + start), endIt(tokens.begin() + (start + tokenCount)) {}

		constexpr std::string_view operator[](size_t index) const noexcept { return *(beginIt + index); }
		constexpr Iterator begin() noexcept { return beginIt; }
		constexpr Iterator end() noexcept { return endIt; }

		size_t tokenCount = 0;
		bool handled = false; // For deferred removal.
		size_t number = 0;
	private:
		Iterator beginIt;
		Iterator endIt;
	};

	struct Equate
	{
		std::string_view identifier;
		std::string_view value;
		uint32_t expandedValue : 24 = 0;
		bool expanded = false;
	};

	bool IsOutputFilepathValid(const std::filesystem::path& filepath, std::string_view& outputName);
	// NOTE: required that all of validExtension is lowercase.
	bool IsExtensionValid(const std::filesystem::path& filepath, std::wstring_view validExtension);
	bool IsASMFile(const std::filesystem::path& filepath);
	bool IsINCFile(const std::filesystem::path& filepath);

	bool ReadFile(const std::filesystem::path& filepath, std::string& fileContents, std::vector<std::string_view>& lines);
	AssemblerError::ID WriteFile(const std::filesystem::path& filepath, std::string_view outputName, const std::vector<uint8_t>& assembly);

	AssemblerError StripWhitespace(std::vector<std::string_view>& lines);
	AssemblerError Tokenize(AssemblerResult& result, const std::vector<std::string_view>& lines, std::vector<std::string_view>& tokens, std::vector<TokenizedLine>& tokenizedLines);
	void CullHandledTokenizedLines(std::vector<TokenizedLine>& tokenizedLines);
	void FindEquates(const std::vector<std::string_view>& tokens, std::vector<TokenizedLine>& tokenizedLines, std::vector<Equate>& equates);

	AssemblerResult Assemble(const AssemblerInfo& info)
	{
		AssemblerResult result;

		if (std::error_code error; !std::filesystem::exists(info.inputFilepath, error) || error)
			return result.Error(AssemblerError_MissingInputFile);

		if (!IsASMFile(info.inputFilepath))
			return result.Error(AssemblerError_InvalidInputFileExtension);

		std::string_view outputName;
		if (!IsOutputFilepathValid(info.outputFilepath, outputName))
			return result.Error(AssemblerError_OutputFileNameInvalid);

		std::string contents;
		std::vector<std::string_view> lines;
		if (!ReadFile(info.inputFilepath, contents, lines))
			return result.Error(AssemblerError_FailedToReadInputFile);

		if (auto error = StripWhitespace(lines))
			return result.Error(error);

		// Tokenize
		std::vector<std::string_view> tokens;
		std::vector<TokenizedLine> tokenizedLines;
		if (auto error = Tokenize(result, lines, tokens, tokenizedLines))
			return result.Error(error);

		// Find equates.
		std::vector<Equate> equates;
		FindEquates(tokens, tokenizedLines, equates);
		CullHandledTokenizedLines(tokenizedLines);

		// This is where the split is from inc and asm files.
		// TODO:
		//	1) Modularize their processes.
		//	2) Recursively call ProcessASMFile for all asm includes and feed their info back up the call stack.


		// TODO: ideas
		//	1) labels that start with a dot are local to the previous non-dotted label
		//		i.e. they can't be referenced from other sections of code either previous
		//		to said non-dotted label or after another non-dotted label.
		//	2) namespaces, eg:
		//		#namespace ti
		//			; ti stuff
		//			#namespace boot
		//				; ti.boot stuff
		//			#endnamespace
		//			; ti stuff
		//		#endnamespace
		//	3) .inc files can only have preprocessor stuff, macros, equates, and include other .inc files.
		//	4) .asm files can do all that .inc files can do, but also have code and export labels.
		//	5) #assert's
		//	6) $ meaning address of current byte

		std::vector<uint8_t> assembly;

		// TODO: generate byte code.

		if (assembly.size() < 2 || (assembly.front() != 0xEF && assembly[1] != 0x7B))
			result.warnings.emplace_back(AssemblerWarning_AssemblyDoesntStartWithEF_7B);

		if (auto error = WriteFile(info.outputFilepath, outputName, assembly))
			return result.Error({ error, lines.size() });

		return result;
	}

	bool IsOutputFilepathValid(const std::filesystem::path& filepath, std::string_view& outputName)
	{
		// Get the filename.
		std::wstring_view filename = std::filesystem::_Parse_filename(filepath.native());

		// Find the dot.
		size_t dotIndex = filename.find(L'.');

		// If the dot does not exist, or there are not enough extension characters following it, filepath is invalid.
		if (dotIndex == std::wstring_view::npos || dotIndex + 4 != filename.size())
			return false;

		// Get the extension.
		std::wstring_view extension = filename.substr(dotIndex + 1, 3);

		// If the extension is invalid, so too is the filepath.
		if (extension.front() != L'8' || util::string::ToLower(extension[1]) != L'x' || util::string::ToLower(extension.back()) != L'p')
			return false;

		// Get the name.
		std::wstring_view name = filename.substr(0, dotIndex);

		// If the name is too small or too large, the filepath is invalid.
		if (name.empty() || name.size() > 8)
			return false;

		// If the name is invalid, so too is the filepath.
		if (!util::string::IsAlpha(name.front()))
			return false;
		for (uint8_t i = 1; i < static_cast<uint8_t>(name.size()); i++)
			if (!util::string::IsAlphanumeric(name[i]))
				return false;

		// Filepath is valid, so convert the name.
		static char outputNameBuffer[8];
		for (uint8_t i = 0; i < 8; i++)
			outputNameBuffer[i] = i < static_cast<uint8_t>(name.size()) ? static_cast<char>(name[i]) : '\0';
		outputName = std::string_view(outputNameBuffer, 8);
		return true;
	}

	bool IsExtensionValid(const std::filesystem::path& filepath, std::wstring_view validExtension)
	{
		// Get the filename.
		std::wstring_view filename = std::filesystem::_Parse_filename(filepath.native());

		// Find the dot.
		size_t dotIndex = filename.find(L'.');

		// If the dot does not exist, or there are not enough extension characters following it, filepath is invalid.
		if (dotIndex == std::wstring_view::npos || dotIndex + validExtension.size() + 1 != filename.size())
			return false;

		// Get the extension.
		std::wstring_view extension = filename.substr(dotIndex + 1, validExtension.size());

		// If the extension is invalid, so too is the filepath.
		for (size_t i = 0; i < validExtension.size(); i++)
			if (util::string::ToLower(extension[i]) != validExtension[i])
				return false;
		return true;
	}

	bool IsASMFile(const std::filesystem::path& filepath)
	{
		return IsExtensionValid(filepath, L"asm");
	}

	bool IsINCFile(const std::filesystem::path& filepath)
	{
		return IsExtensionValid(filepath, L"inc");
	}

	bool ReadFile(const std::filesystem::path& filepath, std::string& contents, std::vector<std::string_view>& lines)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);
		if (!file.is_open())
			return false;

		size_t fileSize = file.tellg();
		file.seekg(0, std::ios::beg);
		contents.clear();
		contents.reserve(fileSize);

		std::istream::sentry sentry(file, true);
		std::streambuf& buffer = *file.rdbuf();

		size_t lineStart = 0;
		auto EndLine = [&contents, &lines, &lineStart](bool addNewLine)
		{
			lines.emplace_back(static_cast<std::string_view>(contents).substr(lineStart, contents.size() - lineStart));
			lineStart = contents.size();
			if (addNewLine)
			{
				contents += '\n';
				lineStart++;
			}
		};

		int c = 0;
		while (c != std::streambuf::traits_type::eof())
		{
			switch (c = buffer.sbumpc())
			{
				case '\n':
				{
					EndLine(true);
					break;
				}
				case '\r':
				{
					if (buffer.sgetc() == '\n')
						buffer.sbumpc();
					EndLine(true);
					break;
				}
				case std::streambuf::traits_type::eof():
				{
					EndLine(false);
					break;
				}
				default:
					contents += static_cast<char>(c);
			}
		}

		return true;
	}

	AssemblerError::ID WriteFile(const std::filesystem::path& filepath, std::string_view outputName, const std::vector<uint8_t>& assembly)
	{
		// This function would not be possible without https://www.ticalc.org/archives/files/fileinfo/247/24750.html.

		constexpr uint16_t dataSectionHeaderSize = (2 + 2 + 1 + 8 + 1 + 1 + 2) + 2;
		constexpr uint16_t maxAssemblySize = (1 << 16) - 1 - dataSectionHeaderSize;

		if (assembly.empty())
			return AssemblerError_AssemblyEmpty;
		if (assembly.size() > maxAssemblySize)
			return AssemblerError_AssemblyTooLarge;

		std::ofstream file(filepath, std::ios::binary);
		if (!file.is_open())
			return AssemblerError_FailedToWriteOutputFile;

		uint16_t dataSectionChecksum = 0;
		auto Write = [&file, &dataSectionChecksum](uint16_t n, bool checksum)
		{
			uint8_t LSB = (n & (0xFF << 0)) >> 0;
			uint8_t MSB = (n & (0xFF << 8)) >> 8;
			file.write((char*)&LSB, 1);
			file.write((char*)&MSB, 1);
			if (checksum)
			{
				dataSectionChecksum += LSB;
				dataSectionChecksum += MSB;
			}
		};

		auto WriteData = [&file, &dataSectionChecksum](uint8_t n)
		{
			file.write((char*)&n, 1);
			dataSectionChecksum += n;
		};

		// Header
		file.write("**TI83F*" "\x01a\x00a\000" "File generated by Shlayne's EZ80Assembler", 8 + 3 + 42);
		uint16_t dataSectionSize = dataSectionHeaderSize + static_cast<uint16_t>(assembly.size());
		Write(dataSectionSize, false);

		// Variable 0 header
		Write(0x000D, true);
		uint16_t variable0Size = static_cast<uint16_t>(assembly.size()) + 2;
		Write(variable0Size, true);
		WriteData(0x06);
		file.write(outputName.data(), 8);
		file.write("\0", 2);
		Write(variable0Size, true);

		// Variable 0 data
		Write(static_cast<uint16_t>(assembly.size()), true);
		for (uint8_t byte : assembly)
			WriteData(byte);

		// Checksum
		Write(dataSectionChecksum, false);

		file.close();
		return AssemblerError_None;
	}

	AssemblerError StripWhitespace(std::vector<std::string_view>& lines)
	{
		for (size_t lineNumber = 0; lineNumber < lines.size(); lineNumber++)
		{
			std::string_view& line = lines[lineNumber];

			if (!line.empty())
			{
				size_t lineEnd = 0;
				if (char c = line.front(); !util::string::FindFirstOfPastQuote(line, lineEnd, c, [](char c) noexcept { return c == ';'; }))
					return { AssemblerError_InvalidStringLiteral, lineNumber };

				size_t lineStart = line.find_first_not_of(" \t\f\v");
				if (lineStart != lineEnd && lineEnd <= line.size())
				{
					line = line.substr(lineStart, lineEnd - lineStart);
					if (!line.empty())
					{
						lineEnd = line.size();
						while (lineEnd > 0 && util::string::IsSpace(line[--lineEnd]));
						line = line.substr(0, lineEnd + 1);
					}
				}
				else if (!line.empty())
					line = {};
			}
		}

		return AssemblerError_None;
	}

	AssemblerError Tokenize(AssemblerResult& result, const std::vector<std::string_view>& lines, std::vector<std::string_view>& tokens, std::vector<TokenizedLine>& tokenizedLines)
	{
		struct TokenizedLineView
		{
			size_t start = 0; // Start index into tokens
			size_t tokenCount = 0; // Number of tokens in this line
			size_t lineNumber = 0;
		};
		std::vector<TokenizedLineView> tokenizedLineViews;

		for (size_t lineNumber = 0; lineNumber < lines.size(); lineNumber++)
		{
			std::string_view line = lines[lineNumber];
			size_t tokenStartIndex = tokens.size();

			if (!line.empty())
			{
				size_t i = 0;
				char c = line.front();

				util::string::FindFirstOf(line, i, c, util::string::IsBlank<char>);
				std::string_view token0 = line.substr(0, i);
				tokens.push_back(token0);

				while (i < line.size() && token0.back() == ':')
				{
					tokenizedLineViews.emplace_back(tokenStartIndex++, 1, lineNumber);
					util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);

					if (i < line.size())
					{
						size_t tokenStart = i;
						util::string::FindFirstOf(line, i, c, util::string::IsBlank<char>);
						token0 = line.substr(tokenStart, i - tokenStart);
						tokens.push_back(token0);
					}
				}

				if (i < line.size())
				{
					if (token0.starts_with('#'))
					{
						std::string_view directive = token0.substr(1);
						if (directive == "include")
						{
							util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };

							// Defer string literal expansion.
							tokens.push_back(line.substr(i, line.size()));
						}
						else if (directive == "define")
						{
							util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };

							// Get identifier.
							size_t tokenStart = i;
							util::string::FindFirstOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };
							tokens.push_back(line.substr(tokenStart, i - tokenStart));

							util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };

							// Get value, either a string literal expansion or a numeric expansion.
							tokenStart = i;
							util::string::FindFirstOf(line, i, c, util::string::IsBlank<char>);
							if (i < line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };
							tokens.push_back(line.substr(tokenStart, i - tokenStart));
						}
						else if (directive == "if" || directive == "elif")
						{
							util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };

							// TODO: handle this stuff.
							// Defer the tokenization because it has operator precedence.
							// Operators in this case are: & ^ | ~ + -(binary) * / % << >> >>> -(unary),
							//	as well as: &&, ||, !, !=, ==, >, <, >=, <=
							tokens.push_back(line.substr(i, line.size()));
						}
						else if (directive == "macro")
						{
							util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };

							// Get identifier.
							size_t tokenStart = i;
							util::string::FindFirstOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };
							tokens.push_back(line.substr(tokenStart, i - tokenStart));

							// Get parameters, identifiers that start with a $.
							do
							{
								util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
								if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };
								if (c != '$') return { AssemblerError_MacroArgsMustStartWithDollarSign, lineNumber };

								tokenStart = i;
								util::string::FindFirstOf(line, i, c, util::string::IsBlank<char>);
								tokens.push_back(line.substr(tokenStart, i - tokenStart));
							}
							while (i < line.size());
						}
						else if (directive == "namespace")
						{
							util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };

							// Get identifier.
							size_t tokenStart = i;
							util::string::FindFirstOf(line, i, c, util::string::IsBlank<char>);
							if (i < line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };
							tokens.push_back(line.substr(tokenStart, i - tokenStart));
						}
						else if (directive == "assert")
						{
							util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };

							// Get the assertion condition, only a numeric expansion.
							size_t tokenStart = i;
							util::string::FindFirstOf(line, i, c, IsParameterSeparator<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };
							tokens.push_back(line.substr(tokenStart, i - tokenStart));

							util::string::FindFirstOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };
							util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidPreprocessorStatement, lineNumber };

							// Defer string literal expansion.
							tokens.push_back(line.substr(i, line.size()));
						}
					}
					else // .directives or instructions, with parameters.
					{
						util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
						if (i >= line.size()) return { AssemblerError_InvalidDotDirectiveOrInstructionParameters, lineNumber };

						if (token0 == ".equ")
						{
							// Get identifier.
							size_t tokenStart = i;
							util::string::FindFirstOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidDotDirectiveParameters, lineNumber };
							tokens.push_back(line.substr(tokenStart, i - tokenStart));

							util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
							if (i >= line.size()) return { AssemblerError_InvalidDotDirectiveParameters, lineNumber };

							tokens.push_back(line.substr(i, line.size()));
						}
						else
						{
							// Get operand 0.
							size_t tokenStart = i;
							util::string::FindFirstOf(line, i, c, IsParameterSeparator<char>);
							tokens.push_back(line.substr(tokenStart, i - tokenStart));

							if (token0 == ".db" || token0 == ".dw" || token0 == ".dl")
							{
								while (i < line.size())
								{
									if (i < line.size() - 2)
									{
										c = line[++i];
										if (util::string::IsBlank(c))
										{
											util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
											if (i >= line.size()) return { AssemblerError_InvalidDotDirectiveParameters, lineNumber };
										}

										tokenStart = i;
										util::string::FindFirstOfPastQuote(line, i, c, IsParameterSeparator<char>);
										tokens.push_back(line.substr(tokenStart, i - tokenStart));
									}
									else // operands end with a comma
									{
										result.warnings.emplace_back(AssemblerWarning_OpcodeTrailingComma, lineNumber);
										break;
									}
								}
							}
							else
							{
								// Optionally get operand 1.
								if (i < line.size())
								{
									if (i < line.size() - 2)
									{
										c = line[++i];
										if (util::string::IsBlank(c))
										{
											util::string::FindFirstNotOf(line, i, c, util::string::IsBlank<char>);
											if (i >= line.size()) return { AssemblerError_InvalidInstructionOpcodes, lineNumber };
										}

										tokenStart = i;
										util::string::FindFirstOf(line, i, c, IsParameterSeparator<char>);
										if (i < line.size())
										{
											if (i < line.size() - 2)
												return { AssemblerError_InvalidInstructionOpcodes, lineNumber };
											else // operands end with a comma
												result.warnings.emplace_back(AssemblerWarning_OpcodeTrailingComma, lineNumber);
										}
										tokens.push_back(line.substr(tokenStart, i - tokenStart));
									}
									else // operands end with a comma
										result.warnings.emplace_back(AssemblerWarning_OpcodeTrailingComma, lineNumber);
								}
							}
						}
					}
				}
				else // Only operandless instructions, parameterless preprocessor statements, label definitions, or syntax errors.
				{
					// TODO: if .directives with no parameters exist, replace the immediate return with their implementations.
					if (token0.starts_with('.')) return { AssemblerError_InvalidDotDirectiveParameters, lineNumber };
					if (token0.starts_with('#'))
					{
						std::string_view directive = token0.substr(1);
						if (directive != "else" && directive != "endif" && directive != "endmacro" && directive != "endnamespace")
							return { AssemblerError_InvalidPreprocessorStatement, lineNumber };
					}
					else if (token0.ends_with(','))
					{
						result.warnings.emplace_back(AssemblerWarning_OpcodeTrailingComma, lineNumber);
						tokens.back() = tokens.back().substr(0, tokens.back().size() - 1);
					}
				}
			}

			if (size_t tokenCount = tokens.size() - tokenStartIndex)
				tokenizedLineViews.emplace_back(tokenStartIndex, tokenCount, lineNumber);
		}

		tokenizedLines.reserve(tokenizedLineViews.size());
		for (auto& tokenizedLineView : tokenizedLineViews)
			tokenizedLines.emplace_back(tokens, tokenizedLineView.start, tokenizedLineView.tokenCount, tokenizedLineView.lineNumber);

		return AssemblerError_None;
	}

	void CullHandledTokenizedLines(std::vector<TokenizedLine>& tokenizedLines)
	{
		std::erase_if(tokenizedLines, [](const TokenizedLine& tokenizedLine) noexcept { return tokenizedLine.handled; });
	}

	void FindEquates(const std::vector<std::string_view>& tokens, std::vector<TokenizedLine>& tokenizedLines, std::vector<Equate>& equates)
	{
		size_t equateCount = 0;

		for (auto& tokenizedLine : tokenizedLines)
			if (tokenizedLine[0] == ".equ")
				equateCount++;

		equates.reserve(equates.size() + equateCount);

		for (auto& tokenizedLine : tokenizedLines)
		{
			if (tokenizedLine[0] == ".equ")
			{
				equates.emplace_back(tokenizedLine[1], tokenizedLine[2]);
				tokenizedLine.handled = true;
			}
		}
	}
}
