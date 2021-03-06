#include "vm/swap.h"
#include <stdio.h>


/* init swap */
void swap_init(void){
  lock_init(&swap_lock);
  swap_block = block_get_role(BLOCK_SWAP);
  //no swap block case
  if(swap_block == NULL){
    printf("NO SWAP DISK\n");
  }
  else{
    //4KB = 8192 Sectors, 1 Page = 8 Sectors
    block_sector_t size = block_size(swap_block)/8;
    swap_bitmap = bitmap_create(size);
    bitmap_set_all(swap_bitmap, 0);
  }
}


/* swap in */
void swap_in(block_sector_t index, void *paddr){
  lock_acquire(&swap_lock);
  //if index is invalid
  if(bitmap_test(swap_bitmap, index) == 0){
    printf("INVALID SWAP BLOCK INDEX\n");
  }
  //read block at paddr
  else{
    unsigned i;
    for(i=0; i<8; i++){
      block_read(swap_block, (index*8)+i, paddr+(i*BLOCK_SECTOR_SIZE));
    }
  }
  bitmap_set(swap_bitmap, index, 0);
  lock_release(&swap_lock);
}


/* swap out */
block_sector_t swap_out(void *paddr){
  lock_acquire(&swap_lock);
  unsigned i;
  bool is_full = true;
  // check whether swap disk is full
  for(i=0; i<bitmap_size(swap_bitmap); i++){
    if(bitmap_test(swap_bitmap, i) == 0){
      is_full = false;
      break;
    }
  }
  // if swap disk is full
  if(is_full){
    printf("SWAP DISK FULL\n");
  }
  //write paddr to block
  else{
    unsigned j;
    for(j=0; j<8; j++){
      block_write(swap_block, (i*8)+j, paddr+(j*BLOCK_SECTOR_SIZE));
    }
  }
  bitmap_set(swap_bitmap, i, 1);
  lock_release(&swap_lock);
  return i;
}
