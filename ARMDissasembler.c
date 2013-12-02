//
//  main.c
//  DisARM
//
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "elf.h"

void Disassemble(unsigned int *armI, int count, unsigned int startAddress);
void DecodeInstruction(unsigned int instr);
void DecodeDataProcessing(unsigned int instr, int ccode);
void FindingCCode(unsigned int condCode);
void FindingOpCode(unsigned int opcode);
int SignExtend(unsigned int x, int bits);
int Rotate(unsigned int rotatee, int amount);

int main(int argc, const char * argv[])
{
    FILE *fp;
    ELFHEADER elfhead;
    ELFPROGHDR *prgHdr;
    int i;
    unsigned int *armInstructions = NULL;
    
    if(argc < 2)
    {
        fprintf(stderr, "Usage: DisARM <filename>\n");
        return 1;
    }
    
    /* Open ELF file for binary reading */
    fp = fopen(argv[1], "rb");
    
    /* Read in the header */
    fread(&elfhead, 1, sizeof(ELFHEADER), fp);
    if(!(elfhead.magic[0] == 0177 && elfhead.magic[1] == 'E' && elfhead.magic[2] == 'L' && elfhead.magic[3] == 'F'))
    {
        fprintf(stderr, "%s is not an ELF file\n", argv[1]);
        return 2;
    }
    
    printf("File-type: %d\n",elfhead.filetype);
    printf("Arch-type: %d\n",elfhead.archtype);
    printf("Entry: %x\n", elfhead.entry);
    printf("Prog-Header: %x\n", elfhead.phdrpos);
    printf("Prog-Header-count: %d\n", elfhead.phdrcnt);
    printf("Section-Header: %x\n", elfhead.shdrpos);
    
    /* Find and read program headers */
    fseek(fp, elfhead.phdrpos, SEEK_SET);
    prgHdr = (ELFPROGHDR*)malloc(sizeof(ELFPROGHDR));
    if(!prgHdr)
    {
        fprintf(fp, "Out of Memory\n");
        fclose(fp);
        return 3;
    }
    
    fread(prgHdr, 1, sizeof(ELFPROGHDR), fp);
    

        printf("Segment-Offset: %x\n", prgHdr[i].offset);
        printf("File-size: %d\n", prgHdr[i].filesize);
        printf("Align: %d\n", prgHdr[i].align);
        
        /* allocate memory and read in ARM instructions */
        armInstructions = (unsigned int *)malloc(prgHdr->filesize + 3 & ~3);
        if(armInstructions == NULL)
        {
            fclose(fp);
            free(prgHdr);
            fprintf(stderr, "Out of Memory\n");
            return 3;
        }
        fseek(fp, prgHdr->offset, SEEK_SET);
        fread(armInstructions, 1, prgHdr->filesize, fp);
        
        /* Disassemble */
        printf("Instructions\n============\n\n");
        
        Disassemble(armInstructions, (prgHdr->filesize + 3 & ~3) /4, prgHdr->virtaddr);
        printf("\n");
        
        free(armInstructions);

    free(prgHdr);
    fclose(fp);

    return 0;
}

void Disassemble(unsigned int *armI, int count, unsigned int startAddress)
{
    int i;
    
    for(i = 0; i < count; i++)
    {
        printf("%08x: %08x ", startAddress + i*4, armI[i]);
        
        DecodeInstruction(armI[i]);
        
        printf("\n");
    }
    
}

void DecodeInstruction(unsigned int instr)
{
    
	unsigned int ccode, rn, op, offset, rot, rd, rs, rm, opcode, instruct, lsInstr;
	ccode = (instr & 0xF0000000) >> 28;
	instruct = (instr & 0x0F000000) >> 24;
	opcode = (instr & 0x01F00000) >> 21;
	rn = (instr & 0x000F0000) >> 16;
        rd = (instr & 0x0000F000) >> 12;
	
	if (instruct == 15){					// SWI Instruction
		instruct = instr & 0x00FFFFFF;
		if (instruct < 5){
			printf("SWI");
			FindingCCode(ccode);
			printf(" %x", instr & 0x0000000F);}}

	if (instruct == 10 || instruct == 11){			// Branch Instruction
		printf("B");
		if (instruct == 11)
			printf("L");
		FindingCCode(ccode);
	//	opcode = (instr & 0x00FFFFFF);
		printf(" %d", SignExtend(instr, 24));}

	if (instruct >= 4 && instruct <= 7)			// Store/Load Instruction
        {
		lsInstr = (instr & 0x00100000) >> 20;
		if (lsInstr == 1)
                	printf("LDR");
		if (lsInstr == 0)
			printf("STR");
		FindingCCode(ccode);

		lsInstr = (instr & 0x00400000) >> 22;
		if (lsInstr == 1)
			printf("B");
		
		lsInstr = (instr & 0x02000000) >> 25;
		offset = (instr & 0x00000FFF);
		if (lsInstr == 1)
			printf(" R%d, [R%d, R%d]", rd, rn, offset);
		if (lsInstr == 0)
		{	
			lsInstr == (instr & 0x00800000) >> 23;
			if (lsInstr == 0)
			{
				printf(" R%d, [R%d]", rd, offset);
			}
			else
			{
				printf(" R%d, R%d, #%d", rd, rn, offset);
			}
		}
        }

	instruct = (instr & 0x0F000000) >> 25;			// Data Instruction
	if (instruct == 0x00000001 || instruct == 0x00000000)
	{
		FindingOpCode(opcode);
		FindingCCode(ccode);
		op = (instr & 0x000000FF);
		if (instruct == 1)
		{
			rot = (instr & 0x00000F00) >> 8;
			if (rot == 0)
			{
				if (opcode == 11 || opcode == 10){
					printf(" R%d, #&%X", rn, op);}
				else if (opcode == 15 || opcode == 13){
                             		printf(" R%d, #&%X", rd, op);}
				else{
                        	        printf(" R%d, R%d, #&%X", rd, rn, op);}
			}
			else
			{
				printf(" R%d, #&%X", rd, Rotate(op, rot*2));
			}
		}
		else
		{
			if (opcode == 11 || opcode == 10)
				printf(" R%d, R%d", rn, op);
			else if (opcode == 15 || opcode == 13)
                                printf(" R%d, R%d", rd, op);
			else
				printf(" R%d, R%d, #&%X", rd, rn, op);
		}
	}

	instruct = (instr & 0x0FC00000) >> 22;
	if (instruct == 0)					//MUL/MLA Instruction
	{
		rd = (instr & 0x000F0000) >> 16;
                rn = (instr & 0x0000F000) >> 12;
                rs = (instr & 0x00000F00) >> 8;
                rm = (instr & 0x0000000F);

		instruct = (instr & 0x00200000) >> 21;
		if (instruct == 0)
		{
			printf("MUL");
			FindingCCode(ccode);
			printf(" R%d, R%d, R%d", rd, rm, rs);
		}
		else if (instruct == 1)
		{
			printf("MLA");
			FindingCCode(ccode);
			printf(" R%d, R%d, R%d, R%d", rd, rm, rs, rn);
		}
	}
}

void FindingCCode(unsigned int condCode)
{
	if (condCode == 0)
		printf("EQ");
	if (condCode == 1)
                printf("NE");
	if (condCode == 2)
                printf("HS");
	if (condCode == 3)
                printf("LO");
	if (condCode == 4)
                printf("MI");
	if (condCode == 5)
                printf("PL");
	if (condCode == 6)
                printf("VS");
	if (condCode == 7)
                printf("VC");
	if (condCode == 8)
                printf("HI");
	if (condCode == 9)
                printf("LS");
	if (condCode == 10)
                printf("GE");
	if (condCode == 11)
                printf("LT");
	if (condCode == 12)
                printf("GT");
	if (condCode == 13)
                printf("LE");
}	

void FindingOpCode(unsigned int opcode)
{
	if (opcode == 0)
		printf("AND");
        if (opcode == 1)
                printf("EOR");
        if (opcode == 2)
                printf("SUB");
        if (opcode == 3)
                printf("RSB");
        if (opcode == 4)
                printf("ADD");
        if (opcode == 5)
                printf("ADC");
        if (opcode == 6)
                printf("SBC");
        if (opcode == 7)
                printf("RSC");
        if (opcode == 8)
                printf("TST");
        if (opcode == 9)
                printf("TEQ");
        if (opcode == 10)
                printf("CMP");
        if (opcode == 11)
                printf("CMN");
        if (opcode == 12)
                printf("ORR");
        if (opcode == 13)
                printf("MOV");
        if (opcode == 14)
                printf("BIC");
        if (opcode == 15)
                printf("MVN");
}
int SignExtend(unsigned int x, int bits)
{
    int r;
    int m = 1U << (bits - 1);
//printf("%x\n", x);
    x = x & ((1U << bits) - 1);
//printf("%x\n", x);
    r = (x ^ m) - m;
//printf("x = %x, m = %x, r = %x\n", x, m, r);
    return r;
}
               
int Rotate(unsigned int rotatee, int amount)
{
    unsigned int mask, lo, hi;

    mask = (1 << amount) - 1;
    lo = rotatee & mask;
    hi = rotatee >> amount;

    rotatee = (lo << (32 - amount)) | hi;
    
    return rotatee;
}

               
        

