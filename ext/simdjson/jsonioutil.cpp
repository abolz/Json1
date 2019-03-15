#include "simdjson/jsonioutil.h"
#include <cstring>
#include <cstdlib>

char * allocate_padded_buffer(size_t length) {
    // we could do a simple malloc
    //return (char *) malloc(length + SIMDJSON_PADDING);
    // However, we might as well align to cache lines...
    size_t totalpaddedlength = length + SIMDJSON_PADDING;
    char *padded_buffer = (char *) aligned_malloc(64, totalpaddedlength);
    return padded_buffer;
}

std::string_view get_corpus(const std::string& filename) {
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
  if (fp != nullptr) {
    std::fseek(fp, 0, SEEK_END);
    size_t len = std::ftell(fp);
    char * buf = allocate_padded_buffer(len);
    if(buf == nullptr) {
      std::fclose(fp);
      throw  std::runtime_error("could not allocate memory");
    }
    std::rewind(fp);
    size_t readb = std::fread(buf, 1, len, fp);
    std::fclose(fp);
    if(readb != len) {
      aligned_free(buf);
      throw  std::runtime_error("could not read the data");
    }
    return std::string_view(buf,len);
  }
  throw  std::runtime_error("could not load corpus");
}
