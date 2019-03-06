#pragma once

#include "bench.h"

bool simdjson_sax_stats(jsonstats& stats, char const* first, char const* last);
bool simdjson_dom_stats(jsonstats& stats, char const* first, char const* last);
