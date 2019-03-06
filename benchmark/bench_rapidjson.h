#pragma once

#include "bench.h"

bool rapidjson_sax_stats(jsonstats& stats, char const* next, char const* last);
bool rapidjson_dom_stats(jsonstats& stats, char const* next, char const* last);
