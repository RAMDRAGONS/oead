// oead_capi.cpp — Thin C wrapper around oead's Yaz0 and SARC for P/Invoke
#include <cstdlib>
#include <cstring>
#include <memory>
#include <oead/sarc.h>
#include <oead/yaz0.h>

#ifdef _WIN32
#define OEAD_EXPORT __declspec(dllexport)
#else
#define OEAD_EXPORT
#endif

extern "C" {

// ============================================================================
// Yaz0
// ============================================================================

OEAD_EXPORT uint8_t* oead_yaz0_decompress(const uint8_t* src, size_t src_size, size_t* out_size) {
  try {
    auto result = oead::yaz0::Decompress({src, src_size});
    *out_size = result.size();
    auto* buf = (uint8_t*)malloc(result.size());
    if (buf)
      memcpy(buf, result.data(), result.size());
    return buf;
  } catch (...) {
    *out_size = 0;
    return nullptr;
  }
}

OEAD_EXPORT uint8_t* oead_yaz0_compress(const uint8_t* src, size_t src_size,
                                        uint32_t data_alignment, int level, size_t* out_size) {
  try {
    auto result = oead::yaz0::Compress({src, src_size}, data_alignment, level);
    *out_size = result.size();
    auto* buf = (uint8_t*)malloc(result.size());
    if (buf)
      memcpy(buf, result.data(), result.size());
    return buf;
  } catch (...) {
    *out_size = 0;
    return nullptr;
  }
}

// ============================================================================
// SARC Reader
// ============================================================================

struct OeadSarc {
  std::vector<uint8_t> data;
  std::unique_ptr<oead::Sarc> sarc;
};

OEAD_EXPORT OeadSarc* oead_sarc_open(const uint8_t* data, size_t size) {
  try {
    auto* s = new OeadSarc();
    s->data.assign(data, data + size);
    s->sarc = std::make_unique<oead::Sarc>(tcb::span<const uint8_t>(s->data));
    return s;
  } catch (...) {
    return nullptr;
  }
}

OEAD_EXPORT void oead_sarc_close(OeadSarc* sarc) {
  delete sarc;
}

OEAD_EXPORT uint16_t oead_sarc_get_num_files(const OeadSarc* sarc) {
  return (sarc && sarc->sarc) ? sarc->sarc->GetNumFiles() : 0;
}

OEAD_EXPORT int oead_sarc_get_file(const OeadSarc* sarc, uint16_t index, const char** out_name,
                                   const uint8_t** out_data, size_t* out_size) {
  if (!sarc || !sarc->sarc)
    return 0;
  try {
    auto file = sarc->sarc->GetFile(index);
    *out_name = file.name.data();
    *out_data = file.data.data();
    *out_size = file.data.size();
    return 1;
  } catch (...) {
    return 0;
  }
}

// ============================================================================
// SARC Writer
// ============================================================================

struct OeadSarcWriter {
  oead::SarcWriter writer;
};

OEAD_EXPORT OeadSarcWriter* oead_sarc_writer_new(int le) {
  auto* w = new OeadSarcWriter();
  w->writer.SetEndianness(le ? oead::util::Endianness::Little : oead::util::Endianness::Big);
  return w;
}

OEAD_EXPORT void oead_sarc_writer_free(OeadSarcWriter* writer) {
  delete writer;
}

OEAD_EXPORT void oead_sarc_writer_set_min_alignment(OeadSarcWriter* writer, size_t alignment) {
  if (writer)
    writer->writer.SetMinAlignment(alignment);
}

OEAD_EXPORT void oead_sarc_writer_add_file(OeadSarcWriter* writer, const char* name,
                                           const uint8_t* data, size_t size) {
  if (!writer)
    return;
  std::vector<uint8_t> vec(data, data + size);
  writer->writer.m_files[name] = std::move(vec);
}

OEAD_EXPORT uint8_t* oead_sarc_writer_write(OeadSarcWriter* writer, size_t* out_size) {
  if (!writer) {
    *out_size = 0;
    return nullptr;
  }
  try {
    auto [alignment, result] = writer->writer.Write();
    *out_size = result.size();
    auto* buf = (uint8_t*)malloc(result.size());
    if (buf)
      memcpy(buf, result.data(), result.size());
    return buf;
  } catch (...) {
    *out_size = 0;
    return nullptr;
  }
}

// ============================================================================
// Memory
// ============================================================================

OEAD_EXPORT void oead_free(void* ptr) {
  free(ptr);
}

}  // extern "C"
