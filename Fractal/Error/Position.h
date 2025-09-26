// Position.h
// Contains the Position Class definition, which is used for tracking relative positions of tokens in source files, useful for clear error outputs
// Copyright (c) 2025-present, Stylianos Kementzetzidis
#pragma once

namespace Fractal {
	struct Position {
		// Path of the file that is currently tracked
		std::filesystem::path sourceFilePath{ "" };

		// Token starting and ending positions in source file
		uint32_t startIndex{ 0 };
		uint32_t endIndex{ 0 };
		uint32_t lineIndexOffset{ 0 };

		// Line of Token in the source file
		uint32_t line{ 0 };
	};
}