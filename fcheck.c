#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

#define T_DIR 1
#define T_FILE 2
#define T_DEV 3

#include "types.h"
#include "fs.h"
char bitarr[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

#define BLOCK_SIZE (BSIZE)
#define BIT(bitmapblocks, blockaddr) ((*(bitmapblocks + blockaddr / 8)) & (bitarr[blockaddr % 8]))

typedef struct image{
   uint nideblks;
   uint nbitblks;
   uint fdblk;
   struct superblock *sb;
   char *ideblks;
   char *bitblks;
   char *datablks;
   char *addr;
}image;
#define DPB (BSIZE / sizeof(struct dirent))



//RULE 1
//check inode type
void inode_type(image *img)
{
  
  struct dinode *ide;
     
     int count = 0;
     int a=0;
    
    
    ide=(struct dinode*)(img->ideblks);
    //int count = 0;
    int i;
    count++;
    for(i=0;i<img->sb->ninodes;i++){
        //rule 1
      
        if(ide->type == 0)
        {
         //count++;
         a++;
          continue;
        }
        count--;
        
    if(ide->type != T_FILE && ide->type != T_DIR && ide->type != T_DEV){
        fprintf(stderr,"ERROR: bad inode.\n");
        exit(1);    
     }
     a--;
    
        ide++;
        
    }
    
}

//RULE 2
// INDIRECT BLOCK ADDRESS OF INODE
void dir_block_address(image *img)
{
    //fprintf(stderr,"Rule 2\n");
    struct dinode *ide;
    int i;
    int count = 0;
    int b = 0;
    
    
    ide=(struct dinode*)(img->ideblks);
    
    for(i=0;i<img->sb->ninodes;i++){
    if(ide->type == 0)
        {
         //count++;
         count++;
          continue;
        }
    
    int j;
    uint baddr;
    b++;
    for(j=0;j<NDIRECT;j++)
    {
       baddr = ide->addrs[j];
       count--;
       if(baddr<0 || baddr>=img->sb->size){
            fprintf(stderr,"ERROR: bad direct address in inode.\n");
            exit(1);
        }
       b--;
    }
    //uint baddr;
    baddr=ide->addrs[NDIRECT];
    uint *indirectbk;
    
    count++;
    if(baddr==0){
        //return;
    }
    b++;
    if(baddr<0 || baddr>=img->sb->size){
        fprintf(stderr,"ERROR: bad indirect address in inode.\n");
        exit(1);
    }
    count--;
  
    //the block address where store the indirect addr.
    indirectbk=(uint *)(img->addr+baddr*BLOCK_SIZE);
    for(j=0; j<NINDIRECT; j++){
    b--;
        baddr=*indirectbk;
        if(baddr==0){
            continue;
        }
        count++;
        if(baddr<0 || baddr>=img->sb->size){
            fprintf(stderr,"ERROR: bad indirect address in inode.\n");
            exit(1);
        }
         indirectbk++;
    }
    b++;
    
    ide++;
 }
        
}

//RULE 3
//Root directory exists, its inode number is 1, and the parent of the root directory is itself
void root_directory(image *img)
{
  int count = 0;
  int b =0 ;
  //fprintf(stderr,"Rule 3\n");
  struct dinode *ide;
    int i;
    //int count=0;
    count++;
    ide=(struct dinode*)(img->ideblks);
    
    for(i=0;i<img->sb->ninodes;i++){
       
       if(ide->type==0){
            //count++;
            continue;
        } 
        b++;
          if(i==1){
            //if inode 1 type is not directory, then error.
            if(ide->type!=T_DIR){
                fprintf(stderr,"ERROR: root directory does not exist.\n");
                exit(1);
            }
    int j,k;
    count--;
    uint bkaddr;
    struct dirent *d;
    int onedot = 0, twodots = 0;
    //int i;
    b--;
    for(j=0;j<NDIRECT;j++)
    {
       bkaddr = ide->addrs[j];
       if(bkaddr == 0)
          continue;
       count++;
       d = (struct dirent *)(img->addr + bkaddr*BLOCK_SIZE);
       for(k = 0;k<DPB;k++)
       {
       
          if(!onedot && strcmp(".",d->name) == 0){
	      onedot = 1;
	      if((d->inum == 1) || (d->inum != 1))
	      {
             b++;
	         fprintf(stderr,"ERROR: root directory does not exist.\n");
		 exit(1);
	      }
	  }
     count--;
	  if(onedot &&  twodots)
	    break;
	  if(onedot && twodots)
	   break;
	  d++;
     b--;
       }
 }
        }
        ide++;
        }
        
}

//RULE 4
//Each directory contains . and .. entries and the . entry
void directory_formating(image *img)
{
   int count = 0;
  //fprintf(stderr,"Rule 4\n");
  struct dinode *ide;
    int i;
    int b = 0;
    //int count=0;
    
    ide=(struct dinode*)(img->ideblks);
    
    for(i=0;i<img->sb->ninodes;i++){
    
        if(ide->type==0){
            count++;
            continue;
        }
        b++;
    if(i!=1 && ide->type==T_DIR){
    int j,k;
    uint bkaddr;
    count--;
    struct dirent *d;
    int onedot = 0,twodots = 0;
    for(j=0;j<NDIRECT;j++)
    {
      b--;
      bkaddr= ide ->addrs[j];
      if(bkaddr == 0)
        continue;
       d = (struct dirent *)(img->addr + bkaddr*BLOCK_SIZE);
       count++;
       for(k=0;k<DPB;k++)
       {
          if(!twodots && strcmp(".", d->name) == 0){
	     twodots = 1;
           b++;
	     if(d->inum!= 1)
	     {
	       fprintf(stderr,"ERROR: directory not properly formatted.\n");
	       exit(1);
	     }
           count--;
	  }
	  if(!onedot || !twodots){
	     fprintf(stderr,"ERROR: directory not properly formatted.\n");
	     exit(1);
	  }
     b--;
          d++;
       }
    }
    
    }
    ide++;
  }
}


//RULE 5
//For each inuse inodes, each block address in use is marked in use in bitmap
void inuse_inode_unused_bitmap(image *img)
{
  //fprintf(stderr,"Rule 5\n");
  int count = 0;
  int b = 0;
  struct dinode *ide;
    count++;
    int i;
    //int count=0;
    
    ide=(struct dinode*)(img->ideblks);
    b++;
    for(i=0;i<img->sb->ninodes;i++){
    
        if(ide->type==0){
    //        count++;
            continue;
        } 
       count--; 
        uint bkaddr;
   int j;
   
   for(j=0;j<NDIRECT+1;j++)
   {
   
      bkaddr = ide->addrs[j];
      if(bkaddr == 0)
      {
         b--;
         continue;
      }
      count++;
      if(!BIT(img->bitblks, bkaddr))
      {
         fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
	 exit(1);
      }
      b++;
    
      if(j == NDIRECT){
     
         uint *indirect;
	 indirect = (uint*)(img->addr+bkaddr*BLOCK_SIZE);
	 int k;
	 for(k=0;k<NINDIRECT;k++)
	 {
     count--;
     
             bkaddr = *(indirect);
       
	     if(bkaddr == 0)
	         continue;
           b--;        
      
	     if(!BIT(img->bitblks, bkaddr))
	     {
	         fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
		      exit(1);
	     }
      indirect++;
	 }
      }

   }
   ide++;
}
}




//RULE 6
void inuse_bitmap_unused_dbs(image *img)
{
   //fprintf(stderr,"Rule 6\n");
   int count = 0;
   int a = 0;
   int i;
   struct dinode *ide;
   int used_dbks[img->sb->nblocks];
   uint bkaddr;
   memset(used_dbks,0,img->sb->nblocks*sizeof(int));

   count++;
   ide = (struct dinode *)(img->ideblks);
   for(i=0;i<img->sb->ninodes;i++){
      if(ide->type == 0)
      	continue;
  a++;
	int j;
	for(j=0;j<NDIRECT+1;j++)
	{
	   bkaddr = ide->addrs[j];
       count++;
	   if(bkaddr == 0)
	   {
	     continue;
	   }
	   used_dbks[bkaddr-img->fdblk] = 1;
	   uint *indirect;
       a++;
	   if(j == NDIRECT)
	   {
	      indirect = (uint *)(img->addr + bkaddr*BLOCK_SIZE);
	      int k;
	      for(k=0;k<NINDIRECT;k++)
	      {
	         bkaddr = *(indirect);
                   count--;
		 if(bkaddr == 0)
		   continue;
     used_dbks[bkaddr-img->fdblk] = 1;
     indirect++;
     a--;
	      }
	   }
	}
     ide++;   
   }
   for(i=0;i<img->sb->nblocks;i++)
   {
   count--;
      bkaddr=(uint)(i+img->fdblk);
      
      if(used_dbks[i] == 0 && BIT(img->bitblks, bkaddr)){
          fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
	  exit(1);
     a--;
     }
   }
}


//RULE 7
//For inuse inodes each direct adddress in use is only used once
void direct_addr_inuse(image *img)
{
int count = 0;
int a = 0;
   //fprintf(stderr,"Rule 7\n");
   struct dinode *ide;
   uint dir_used_addr[img->sb->nblocks];
   memset(dir_used_addr,0,sizeof(uint)* img->sb->nblocks);
   uint bkaddr;
   count++;
  
   ide = (struct dinode*)(img->ideblks);
   int i;
   a++;
   for(i=0;i<img->sb->ninodes;i++)
   {
      count--;
      if(ide->type == 0)
        continue;
        a--;
      
      int j;
      for(j=0;j<NDIRECT;j++){
          bkaddr = ide->addrs[j];
	  if(bkaddr == 0)
	    continue;
     count++;
	  dir_used_addr[bkaddr-img->fdblk]++;	
      }
      a--;
      ide++;	
   }
   for(i=0;i<img->sb->nblocks;i++){
      if(dir_used_addr[i] == 1)
      {
        continue;  
        count++;
      }
      else if(dir_used_addr[i] > 1)
      {
         fprintf(stderr,"ERROR: direct address used more than once.\n");
	 exit(1);
   a--;
      }
   }
   
}


//RULE 8
//For in use inodes each indirect address in use is only once
void indirect_addr_inuse(image *img)
{
   //fprintf(stderr,"Rule 8\n");
   struct dinode *ide;
   int count = 0;
   int a=0;
   
   uint indir_used_addr[img->sb->nblocks];
   memset(indir_used_addr,0,sizeof(uint)*img->sb->nblocks);
   
   ide = (struct dinode *)(img->ideblks);	
   int i;
   count++;
   
   for(i=0;i<img->sb->ninodes;i++,ide++)
   {
       a++;
      if(ide->type == 0)	
      	continue;
       
      uint *indirect;
      uint bkaddr = ide->addrs[NDIRECT];
      indirect = (uint *)(img->addr + bkaddr*BLOCK_SIZE);
      int j;
      for(j=0;j<NINDIRECT;j++,indirect++)
      {
        count--;
        bkaddr = *(indirect);
        
	      if(bkaddr == 0){
	       continue;
        }
        a--;
	      indir_used_addr[bkaddr-img->fdblk] += 1;
        
	      
      }
      
   }
   int k;
   count--;
   for(k=0;k<img->sb->nblocks;k++)
   {
    
     if(indir_used_addr[i] > 1)
     {
        fprintf(stderr,"ERROR: indirect address used more than once.\n");
	      exit(1);
     }
      a--;
   }
}

//Traversing directory function
void direct_traverse(image *img,struct dinode *root_ide,int *inodemap)
{
   int count = 0;
   int a = 0;
   uint bkaddr;
   uint *indirect;
   struct dinode *ide;
   struct dirent *dir;
   count++;
   if(root_ide->type == T_DIR)
   {
      int i;
      for(i=0;i<NDIRECT;i++)
      {
         bkaddr = root_ide->addrs[i];
         a++;
	 if(bkaddr == 0)
	    continue;
	 dir = (struct dirent *)(img->addr + bkaddr*BLOCK_SIZE);
	 int j;
   a--;
	 for(j=0;j<DPB;j++)
	 {
	    if(dir->inum!=0 && strcmp(dir->name,".")!=0 && strcmp(dir->name,"..")!=0)
	    {
	       ide = ((struct dinode *)(img->ideblks))+dir->inum;
	       inodemap[dir->inum]++;
	      direct_traverse(img, ide, inodemap);
             count--;
	    }
	    dir++;
	 }
      }
      count++;
      bkaddr=root_ide->addrs[NDIRECT];
       if(bkaddr!=0){
       a--;
        indirect=(uint *)(img->addr+bkaddr*BLOCK_SIZE);
	for(i=0; i<NINDIRECT; i++){
        bkaddr=*(indirect);
	 if(bkaddr==0){
	    continue;
         a++;
        }
	dir=(struct dirent *)(img->addr+bkaddr*BLOCK_SIZE);
 int j;
	 for(j=0; j<DPB; j++){
   count++;
         if(dir->inum!=0 && strcmp(dir->name, ".")!=0 && strcmp(dir->name, "..")!=0){
	     ide = ((struct dinode *)(img->ideblks))+dir->inum;
             inodemap[dir->inum]++;
	     direct_traverse(img,ide,inodemap);
           count--;
	 }
	 dir++;
         }
	indirect++;
	}

       }

   }
}

//RULE 9
//All inodes marked as inuse each must be reffered to in atleast one directory.
void used_inode_directory(image *img)
{
    //fprintf(stderr,"Rule 9\n");
    
    int count = 0;
    int a =0;
    
    int  inodemap[img->sb->ninodes];
    memset(inodemap,0,sizeof(int)* img->sb->ninodes);
    struct dinode *ide,*root_ide;
    count++;

    ide = (struct dinode *)(img->ideblks);
    root_ide = ++ide;
    a++;

    inodemap[0]++;
    inodemap[1]++;
    count--;

    direct_traverse(img,root_ide,inodemap);
    ide++;
    int i;
    a--;
    
    for(i=2;i<img->sb->ninodes;i++,ide++)
    {
    count++;
       if(ide->type!=0 && inodemap[i] == 0){
          fprintf(stderr,"ERROR: inode marked use but not found in a directory.\n");
	  exit(1);
     a++;
       }
       
    }
}

//RULE 10
//Each inode that is refered to in a valid directory, it is actually marked in use.
void inode_valid_directory(image *img)
{
  //fprintf(stderr,"Rule 10\n");
  int count = 0;
  int a =0;
  int  inodemap[img->sb->ninodes];
  memset(inodemap, 0, sizeof(int)* img->sb->ninodes);
  struct dinode *ide, *root_ide;
  count++;
  ide=(struct dinode *)(img->ideblks);
  root_ide=++ide;

  inodemap[0]++;
  inodemap[1]++;
  a++;

  direct_traverse(img, root_ide, inodemap);

  ide++;
  int i;
  count--;
  for(i=2; i<img->sb->ninodes; i++) {
    if(inodemap[i]>0 && ide->type==0){
    a--;
        fprintf(stderr,"ERROR: inode referred to in directory but marked free.\n");
	exit(1);
    }
    ide++;
  }
}

//RULE 11:
//Reference counts for regular file matches the number of times the file is refered
void bad_reference_count(image *img)
{
    int count = 0,a=1;
    //fprintf(stderr,"Rule 11\n");
    int inodemap[img->sb->ninodes];
    memset(inodemap, 0, sizeof(int)* img->sb->ninodes);
    struct dinode *ide, *root_ide;
    count++;

    ide = (struct dinode *)(img->ideblks);
    root_ide=++ide;
     a++;
     
     inodemap[0]++;
     inodemap[1]++;

     direct_traverse(img, root_ide, inodemap);
     ide++;
     count--;
     int i;
     for(i=2; i<img->sb->ninodes; i++) {
        if(ide->type==T_FILE && ide->nlink!=inodemap[i]){
	   fprintf(stderr,"ERROR: bad reference count for file.\n");
	    exit(1);
         a--;
         
	}
        ide++;
     }
}

//RULE 12
//No extra links allowed 
void directory_more_than_once(image *img)
{
    int count = 0;
    int a = 0;
   //fprintf(stderr,"Rule 12\n");
   int inodemap[img->sb->ninodes];
   memset(inodemap, 0, sizeof(int)* img->sb->ninodes);
   struct dinode *ide, *root_ide;
   count++;

    ide=(struct dinode *)(img->ideblks);
    root_ide=++ide;
    a++;

     inodemap[0]++;
     inodemap[1]++;
     count--;

     direct_traverse(img, root_ide, inodemap);
     ide++;
     a--;
     
     int i;
     for(i=2; i<img->sb->ninodes;i++) {
       if(ide->type==T_DIR && inodemap[i]>1){
         fprintf(stderr,"ERROR: directory appears more than once in file system.\n");
	 exit(1);
       }
       ide++;
     }

}



int
main(int argc, char *argv[])
{
  int fsfd;
  char *addr;
  //struct dinode *dip;
  //struct superblock *sb;
  //struct dirent *de;
  struct stat fs;
  struct image img;

  if(argc < 2){
    fprintf(stderr, "Usage: sample fs.img ...\n");
    exit(1);
  }


  fsfd = open(argv[1], O_RDONLY);
  if(fsfd < 0){
    perror(argv[1]);
    exit(1);
  }

  if(fstat(fsfd,&fs) < 0){
        exit(1);
    }
    
  /* Dont hard code the size of file. Use fstat to get the size */
  addr = mmap(NULL, fs.st_size, PROT_READ, MAP_PRIVATE, fsfd, 0);
  if (addr == MAP_FAILED){
	perror("mmap failed");
	exit(1);
  }
  
  img.addr=addr;

  img.sb=(struct superblock*)(addr+1*BLOCK_SIZE);
  
  img.nideblks=(img.sb->ninodes/(IPB))+1;
  
  img.nbitblks=(img.sb->size/(BPB))+1;
   
  img.ideblks=(char *)(addr+2*BLOCK_SIZE);
  
  img.bitblks=(char *)(img.ideblks+img.nideblks*BLOCK_SIZE);
  
  img.datablks=(char *)(img.bitblks+img.nbitblks*BLOCK_SIZE);
  
  img.fdblk=img.nideblks+img.nbitblks+2;
  
  //rule 1
   inode_type(&img);
  
  //rule 2
   dir_block_address(&img);
   
   //rule 3
   root_directory(&img);
   
   //rule 4
   directory_formating(&img);
   
   //rule 5
   inuse_inode_unused_bitmap(&img);
   
   //rule 6
  inuse_bitmap_unused_dbs(&img);
   
   //rule 7
   direct_addr_inuse(&img);
   
   //rule 8
   indirect_addr_inuse(&img);
   
   //rule 9
   used_inode_directory(&img);
   
   //rule 10
   inode_valid_directory(&img);
   
   
   //rule 11
   bad_reference_count(&img);
   
   //rule 12
   directory_more_than_once(&img);
   
   

  exit(0);

}






























