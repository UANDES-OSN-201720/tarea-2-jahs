/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

const char *algth;
struct disk *disk;
int *frames_t;
int contadorfaltas;
int contadorlecturas;
int contadorescrituras;

void page_fault_handler( struct page_table *pt, int page )
{
	contadorfaltas++;
	//printf("page fault on page #%d\n",page);
	int nframes = page_table_get_nframes(pt);
	char *physmem = page_table_get_physmem(pt);
	int frame = 0; //Marco donde se cargara la pagina.
	int randoms;
	//Se elige el marco dependiendo del algoritmo.
	if(!strcmp(algth,"rand")) {
		frame = lrand48()%nframes;	
	}else if (!strcmp(algth,"fifo")){
		frame = page%nframes;
	}else if (!strcmp(algth,"custom")){
		//aleatorio pero con paridad
		randoms=lrand48()%nframes;
		if (page%2==0){
		
			while((randoms)%2!=0){
				randoms=lrand48()%nframes;
			}
			
		}else{
			while((randoms)%2==0){
				randoms=lrand48()%nframes;
			}
		}
		frame=randoms;
	}else{
		fprintf(stderr,"unknown algorithm: %s\n",algth);
		exit(1);
	}

	//Si hay una pagina cargada en el marco, la escribo al disco.
	if (frames_t[frame]!=-1){
		contadorescrituras++;
		disk_write(disk, frames_t[frame], &physmem[frame]);						
	}

	//Leo la pagina del disco y la cargo en el marco.
	disk_read(disk, page, &physmem[frame]);
	contadorlecturas++;
	frames_t[frame]=page;
	page_table_set_entry(pt,page,frame,PROT_WRITE);
	//page_table_print(pt);
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		/* Add 'random' replacement algorithm if the size of your group is 3 */
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	algth = argv[3];
	const char *program = argv[4];
	srand48(time(NULL));

	frames_t = malloc(nframes*sizeof(int)); //Tabla de largo del numero de marcos. Guarda que pagina esta cargada en cada marco.
	//En un principio, la cargamos con -1.
	for (int i = 0; i<nframes;i++){
		frames_t[i]=-1;
	}

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}
	
	

	char *virtmem = page_table_get_virtmem(pt);

	char *physmem = page_table_get_physmem(pt);

	

	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);
		
	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[4]);

	}

	printf("número de faltas de paginas: %d\n",contadorfaltas);
	printf("número de lecturas a disco: %d\n",contadorlecturas);
	printf("número de escrituras a disco: %d\n",contadorescrituras);

	page_table_delete(pt);
	disk_close(disk);
	return 0;
}
