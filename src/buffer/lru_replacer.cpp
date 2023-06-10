#include "buffer/lru_replacer.h"

LRUReplacer::LRUReplacer(size_t num_pages){}

LRUReplacer::~LRUReplacer() = default;

/**
 * Student Implement
 */
bool LRUReplacer::Victim(frame_id_t *frame_id) {
  if (lru_list.empty()) {
    frame_id = nullptr;
    return false;
  }
  *frame_id = lru_list.front();
  lru_list.pop_front();
  lru_map.erase(*frame_id);
  return true;
}

/**
 * Student Implement
 */
void LRUReplacer::Pin(frame_id_t frame_id) {
  if (lru_map.find(frame_id) != lru_map.end()) {
    lru_list.erase(lru_map[frame_id]);
    lru_map.erase(frame_id);
  }
}

/**
 * Student Implement
 */
void LRUReplacer::Unpin(frame_id_t frame_id) {
  if (lru_map.find(frame_id) == lru_map.end()) {
    lru_list.push_back(frame_id);
    lru_map.emplace(frame_id, --lru_list.end());
  }
}

/**
 * Student Implement
 */
size_t LRUReplacer::Size() {
  return lru_map.size();
}