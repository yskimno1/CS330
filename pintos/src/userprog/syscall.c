#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include <user/syscall.h>
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include <string.h>
#include "threads/malloc.h"
#include "userprog/process.h"

static void syscall_handler (struct intr_frame *);


static void get_args(int *esp, int *args);
static void check_addr(const void *vaddr);
static struct file *get_file_by_fd(int fd);

static struct lock file_lock;


//to save in file_list in thread
struct file_list_elem{
  int fd;
  struct file *f;
  struct list_elem elem;
};

void
syscall_init (void) 
{
  lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  
  int *esp= f->esp;
  check_addr(esp);
  int syscall = *esp;
  int args[3];
  get_args(esp, args);
  //printf ("system call!\n");
  //printf("current esp : %p\n", esp);
  //printf("current esp + 1 : %p\n", esp+1);

  switch(syscall){
  	case SYS_HALT:
  		printf("\nSYS_HALT\n");
      halt();
  		break;
  	case SYS_EXIT:
  		printf("\nSYS_EXIT\n");
      
      exit(args[0]);
  		break;

  	case SYS_EXEC:
  		printf("\nSYS_EXEC\n");
      f->eax = exec((const char*) args[0]);
  		break;

  	case SYS_WAIT:
  		printf("\nSYS_WAIT\n");
  		break;

  	case SYS_CREATE:
  		printf("\nSYS_CREATE\n");
  		break;

  	case SYS_REMOVE:
  		printf("\nSYS_REMOVE\n");
  		break;

  	case SYS_OPEN:
  		printf("\nSYS_OPEN\n");
      lock_acquire(&file_lock);
      f->eax = open((const char *)args[0]);
  		break;

  	case SYS_FILESIZE:
  		printf("\nSYS_FILESIZE\n");
      lock_acquire(&file_lock);
      f->eax = filesize(args[0]);
  		break;

  	case SYS_READ:
  		printf("\nSYS_READ\n");
  		break;

  	case SYS_WRITE:
  		printf("\nSYS_WRITE\n");
      //get_args(esp, args, 3);
      lock_acquire(&file_lock);
      f->eax = write(args[0], (void *) args[1], (unsigned) args[2]);
      lock_release(&file_lock);
  		break;

  	case SYS_SEEK:
  		printf("\nSYS_SEEK\n");
  		break;

  	case SYS_TELL:
  		printf("\nSYS_TELL\n");
  		break;

  	case SYS_CLOSE:
  		printf("\nSYS_CLOSE\n");
  		break;
  }

}




void halt (void){
	shutdown_power_off();
}


void exit (int status){
  //need to do file close

  thread_current()->exit_status = status;
  thread_exit();
}




pid_t exec (const char *file){
  tid_t tid = process_execute(file);
  struct thread *t = thread_current();
  if(t->exit_status == -1)
    return -1;
  else
    return (pid_t) tid;
}
/*

int wait (pid_t){

}

bool create (const char *file, unsigned initial_size){

}

bool remove (const char *file){

}
*/


int open (const char *file){
  check_addr((const void *) file);
  struct file *f = filesys_open(file);
  
  if(f == NULL){
    lock_release(&file_lock);
    return -1;
  }
  else{
    struct file_list_elem *fle = malloc(sizeof(struct file_list_elem));
    fle->f = f;
    fle->fd = thread_current()->fd_count;
    thread_current()->fd_count++;
    list_push_back(&thread_current()->file_list, &fle->elem);
    lock_release(&file_lock);
    return fle->fd;
  }
}



int filesize (int fd){
  struct file *f = get_file_by_fd(fd);
  //if there is no file with fd
  if(f==NULL){
    lock_release(&file_lock);
    return -1;
  }
  //if there is file with fd
  else{
    lock_release(&file_lock);
    return file_length(f);
  }
}

/*
int read (int fd, void *buffer, unsigned length){
  check_addr(buffer);
  if(fd == 0){

  }
}
*/



int write (int fd, const void *buffer, unsigned length){
  check_addr(buffer);
  if(fd == 1){
    putbuf(buffer, length);
  }
  else{

  }
  return length;
}



/*
void seek (int fd, unsigned position){

}
unsigned tell (int fd){

}
void close (int fd){

}
*/


//to get arguments of current esp
static void get_args(int *esp, int *args){
  int i;
  int *temp_ptr;
  for(i=0; i<3; i++){
    temp_ptr = esp + 1 + i;
    check_addr((const void *) temp_ptr);
    args[i] = *temp_ptr;
  }
  /*
  for(i=0; i<count; i++){
    printf("%d: %p\n", i, args[i]);
  }
  */
}

static void check_addr(const void *vaddr){
	if(is_kernel_vaddr(vaddr)){
		printf("not valid addr : %p\n", vaddr);
	}
}



//get current thread's file list and find file by fd
static struct file *get_file_by_fd(int fd){
  struct thread *t = thread_current();
  struct list file_list = t->file_list;
  struct list_elem *e;
  for(e=list_begin(&file_list); e!=list_end(&file_list); e=list_next(e)){
    struct file_list_elem *fle = list_entry(e, struct file_list_elem, elem);
    if(fle->fd == fd){
      return fle->f;
    }
  }

  return NULL;
}

