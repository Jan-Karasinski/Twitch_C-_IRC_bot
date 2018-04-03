#include "stdafx.h"
#include "Logger.h"

std::ofstream PrimitiveLogger::log{ "..\log.txt", std::ios::app };