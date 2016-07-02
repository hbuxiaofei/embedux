#ifndef __HEAP_H__
#define __HEAP_H__

struct heap_node {
  struct heap_node* left;
  struct heap_node* right;
  struct heap_node* parent;
};

struct heap {
  struct heap_node* min;
  unsigned int nelts;
};

/* Return non-zero if a < b. */
typedef int (*heap_compare_fn)(const struct heap_node* a,
                               const struct heap_node* b);


/* Public functions. */
void heap_init(struct heap* heap);
struct heap_node* heap_min(const struct heap* heap);
void heap_insert(struct heap* heap, struct heap_node* newnode, heap_compare_fn less_than);
void heap_remove(struct heap* heap, struct heap_node* node, heap_compare_fn less_than);
void heap_dequeue(struct heap* heap, heap_compare_fn less_than);




#endif

