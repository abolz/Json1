#pragma once

#include "bench.h"

bool sajson_sax_stats(jsonstats& stats, char const* first, char const* last);
bool sajson_dom_stats(jsonstats& stats, char const* first, char const* last);
