#pragma once

#include "bench.h"

bool json1_sax_stats(jsonstats& stats, char const* first, char const* last);
bool json1_dom_stats(jsonstats& stats, char const* first, char const* last);
