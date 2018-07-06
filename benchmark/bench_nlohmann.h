#pragma once

#include "jsonstats.h"

bool nlohmann_sax_stats(jsonstats& stats, char const* first, char const* last);
bool nlohmann_dom_stats(jsonstats& stats, char const* first, char const* last);
