#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Error/Error.h"

namespace Fractal {
	enum class Size : uint8_t {
        None = 0,
		Byte = 1,
		Word = 2,
		DWord = 4,
		QWord = 8
	};
}