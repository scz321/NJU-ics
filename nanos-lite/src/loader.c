#include <proc.h>
#include <elf.h>
#include <stdio.h>//用于支持FILE结构体

#include <stdlib.h>//为了调用malloc，谁让你自己没有实现呢qwq

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif


extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);

extern uint8_t ramdisk_start;
extern uint8_t ramdisk_end;

// static uintptr_t loader(PCB *pcb, const char *filename) {
//   //TODO();
//   //pa3 ，第一版实现，读取build目录下手动复制过来的ramdisk.img（路径为:../build/ramdisk.img
//   //我们首先以二进制方式打开文件，使用Elf_Ehdr解析得到elf文件头，从中提取出Elf_Phdr的起始地址、e_phentsize、
//   //以及e_phnum。
//   //然后遍历每一项Elf_Phdr，从中提取出每一个segment的Offset、VirtAddr、FileSiz、MemSiz
//   //uint8_t buf[MAX_ELF]=fopen();
//   FILE* fp=fopen("../build/ramdisk.img","rb");//二进制方式读取
// 	//获取文件内容以及文件头
// //-------------------bad practice----------------
// 	// Elf_Ehdr* eheader=NULL;
// 	// for(int i=0;;i++){
// 	// 	if(fread(&buf[i],sizeof(uint8_t),1,fp)!=sizeof(uint8_t))
// 	// 	break;
// 	// }
// 	// eheader=(Elf_Ehdr*) buf;
// 	//
// //--------------------------good practice------------
// 	fseek(fp,0,SEEK_END);
// 	ftell(fp);
// 	long filesize = ftell(fp);
// 	fseek(fp,0,SEEK_SET);

// 	//这样，就可以直接一次fread读完了，并且buf的分配也是动态的。一举两得
// 	uint8_t* buf=malloc(filesize);
// 	if(fread(buf,sizeof(uint8_t),filesize,fp)!=sizeof(uint8_t)){
// 		assert(0);
// 	}
// 	Elf_Ehdr* eheader = (Elf_Ehdr*) buf;
	

// 	//解析，得到我们期望的PHT相关信息
// 	Elf32_Off	ptblOff=eheader->e_phoff;
// 	Elf32_Half ptblSize=eheader->e_phentsize;
// 	Elf32_Half ptblNum=eheader->e_phnum;


// 	//遍历每一项，对每一项，提取出Elf32_Off	p_offset;Elf32_Addr	p_vaddr;Elf32_Word	p_filesz;
// 	//Elf32_Word	p_memsz;四项，调用ramdisk_write写入nemu的对应虚拟地址处。

// 	for (int i = 0; i < ptblNum; i++)
// 	{
// 		Elf_Phdr *phdr = (Elf_Phdr *)(buf + ptblOff + i * ptblSize);
// 		if (phdr->p_type == PT_LOAD)
// 		{
// 			// Check if the segment should be loaded
// 			ramdisk_write(buf + phdr->p_offset, phdr->p_vaddr, phdr->p_filesz);

// 			// If the segment's memory size is larger than its file size, clear the extra memory
// 			if (phdr->p_memsz > phdr->p_filesz)
// 			{
// 				size_t clear_len = phdr->p_memsz - phdr->p_filesz;
// 				uint8_t *clear_start = (uint8_t *)((uintptr_t)ramdisk_start + phdr->p_vaddr + phdr->p_filesz);
// 				memset(clear_start, 0, clear_len);
// 			}


// 		}
// 	}

//   return eheader->e_entry;
// }


//-------------------手搓版本qwq（参考）

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);
static void read(int fd, void *buf, size_t offset, size_t len){
  fs_lseek(fd, offset, SEEK_SET);
  fs_read(fd, buf, len);
}

static uintptr_t loader(PCB *pcb, const char *filename) {
  //int fd = fs_open(filename, 0, 0);
  //int fd = fs_open("../build/ramdisk.img", 0, 0);//这里暂时写死
  
   int fd = fs_open(filename, 0, 0);
  
  
  Elf_Ehdr elf_header;
  read(fd, &elf_header, 0, sizeof(elf_header));
  //根据小端法 0x7F E L F
  assert(*(uint32_t *)elf_header.e_ident == 0x464c457f);
  
  Elf32_Off program_header_offset = elf_header.e_phoff;
  size_t headers_entry_size = elf_header.e_phentsize;
  int headers_entry_num = elf_header.e_phnum;

  for (int i = 0; i < headers_entry_num; ++i)
  {
	  Elf_Phdr section_entry;
	  read(fd, &section_entry,
		   i * headers_entry_size + program_header_offset, sizeof(elf_header));
	  void *virt_addr;
	  switch (section_entry.p_type)
	  {
	  case PT_LOAD:
		  virt_addr = (void *)section_entry.p_vaddr;
		  read(fd, virt_addr, section_entry.p_offset, section_entry.p_filesz);
		  memset(virt_addr + section_entry.p_filesz, 0,
				 section_entry.p_memsz - section_entry.p_filesz);
		  break;

	  default:
		  break;
	  }
  }

  return elf_header.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = 0x%08x\n", entry);
  ((void(*)())entry) ();
}

