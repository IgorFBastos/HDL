/******************************************************************************
*  This is a C program that reads the MARS MIPS simulator Text and Data Memory
*	dumps and generates a VHDL file (memory.vhd) containing two
*	entity/architecture pairs declarations, which define Instruction and Data
*	memory blocks
*	Observations:
*		1) The dumps are assumed to contain hexadecimal values only
*		for addresses, object codes and data. Parameterize MARS to generate
*		dumps adequately
*		2) The memory sizes are fixed, 1 16-Kbit Block for instructions and
*			64 Kbits for Data.
*
*  FERNANDO GEHM MORAES - 16/Nov/2007 - First version (for the PCSPIM sim)
*  NEY LAERT VILAR CALAZANS - 16/Nov/2007
* 		- Software changed to support MARS memory dumps
*  FERNANDO MORAES - 30/Oct/2012
*		- Changed to accept two native MARS dump files, without edition
*  FERNANDO MORAES - 01/Jun/2013
*		- Change to use function getline2 instead of getline
*  NEY CALAZANS - 12/Jun/2013
*		- Small documentational bug in the software Help solved
*  NEY CALAZANS - 14-15/Feb/2025
*		(1) Data memory architectural bug that generated incorrect operation
*			for instruction SB corrected
*       (2) Data memory architectural bug in chip enable signals connections
*			(all modules were connected to w3 only) corrected
*       (3) Data memory architectural bug that wrongly chose the block where
*			to perform a single byte write in SB instruction corrected
*  NEY CALAZANS - 24/Apr/2025
*		- The polarity of the bw signal changed to adapt to the current
*			revision of the MIPS_S architecture implementation bw='1' now
*			means a single byte is to be written in memory
*		- Minor change: The names of the memory architectures are the same
*			as their entity names
*		- Program comments are now all in English
*  NEY CALAZANS - 1ro/May/2025
*       - A comment warning was added to remember that in the Data Memory
*           the ce input signal, although  it exists, it is currently unused
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#define TAM 250

int separador(char car)
 {
  switch(car)
   { case ' ': case '\n': case '\t': case '\r':   case '/':
     case '*': case '=':  case ',':   case '.':  return 1;
     default: return 0;
   }
}

/******************************************
* Return a string from the file line read *
******************************************/
int search_word(char *word, int *counter, char *token)
{
    int             k = 0;

    while (separador(word[*counter]))
        (*counter)++;
    while (!separador(word[*counter]) && word[*counter] != '\0')
        token[k++] = word[(*counter)++];
    token[k] = '\0';
    return k;
}

/**********************************************************************/
 // Routine to read a line from dump files - Fernando Moraes
/**********************************************************************/
int getline2(word, limit, p1)
    char           *word;
    FILE           *p1;
    int             limit;
/**********************************************************************/
{   int             i = 0;
    char            c;

    while ((c = getc(p1)) != '\n') {
        if (c == EOF)
            return 0;
        word[i] = c;
        if(++i==TAM)
             { printf("\ERROR: line too large %s %d\n", word, (int) strlen(word));
               fflush(stdout);
               exit(1);
             }
    }

    word[i] = 0x00;   // TIRA O \n  DA STRING
    return 1;
}

//-------------------------------------------------------------------------------
int main(int argc, char *argv[])
//-------------------------------------------------------------------------------
{
  FILE *fp, *out;
  int i,j, palavra, offset, b, DEBUG;
  char line[TAM], wd[TAM], memoria_dados[64][65],  ram0[64][65], ram1[64][65], ram2[64][65], ram3[64][65];
  time_t clock;

  clock = time(NULL);

  if (argc==1) {
     puts("***********************************************************************************");
     puts("*         Program which reads two files: one with MIPS_S object code instructions *");
     puts("*         for the program to execute and one containing the data for the program  *");
   printf("*         usage: %12s <program instructions file> <program data file> [d]         *\n", argv[0]);
     puts("*         [d] can be any optional printable char. If it exists indicates debugging*");
     puts("*         Please report bugs to fernando.moraes@pucrs.br and nlvcalazans@gmail.com*");
     puts("**********************************************************************************\n");
     return(0);
  }

	// initializes strings related to memories
	// 	- 1 BRAM with 512 32-bit words --> Instruction Memory
	//	- 4 BRAMS, interleaved to create a 2K 32-bit words accessible at word frontier
	//		or at single-byte for both reading and writing
  for(i=0; i<64; i++)
    { for(j=0; j<64; j++)
           { memoria_dados[i][j]='0';
             ram0[i][j]='0';
             ram1[i][j]='0';
             ram2[i][j]='0';
             ram3[i][j]='0';
           }
      memoria_dados[i][64]='\0';
      ram0[i][64]='\0';
      ram1[i][64]='\0';
      ram2[i][64]='\0';
      ram3[i][64]='\0';
    }

  out = fopen("memory.vhd", "w");

  if (argc==4)  DEBUG=1;  else  DEBUG=0;

  //////////////////
  /////////////////    READ INSTRUCTION FILE
  /////////////////
  if ((fp = fopen(argv[1], "r")) == NULL) {
            printf("Error while opening file %s !\n", argv[1]);
            return 0;
        }

  puts("---------------------- PROCESSING INSTRUCTIONS ");

  offset = 56 ;
  palavra = 0;

  // verifies if the data file IS the data file
  getline2(line, TAM, fp);
  i = 0;
  search_word(line, &i, wd);
  if( strcmp(wd,"Address")) {
  	 printf("\nThe first word of the instruction file is %s and not Address - maybe you changed the input file order... - REVIEW! - not generating memory.vhd\n\n", wd);
  	 return(0);
   }

  while (getline2(line, TAM, fp))
    {
      i = 0;                         // points to the line start
      search_word(line, &i, wd);     // wd is a pointer to the next token

      if (wd[0]=='0' &&  wd[1]=='x' )
		  // the program line starts with "0x" ==> valid address
           {
             if (DEBUG) printf("Program memory address %10s ", wd);
             search_word(line, &i, wd);      // code
             if (DEBUG) printf("with opcode %10s\n", wd);
             // Transfer to the Data Memory (Output generation)
             for(b=0; b<8; b++)
                  memoria_dados[palavra][offset+b] = wd[b+2];
					// "+2" passes over "0x"
              offset = offset - 8;
              if( offset < 0 )
                { offset = 56 ;
				//	Displacement inside the string relative to the memory
				//	The string goes from 0 to 63
                  palavra ++ ;
                }
            }

     }
  fclose(fp);

  //////////////////
  /////////////////    READ DATA FILE
  /////////////////
  if ((fp = fopen(argv[2], "r")) == NULL) {
            printf("Error while opnening file %s !\n", argv[2]);
            return 0;
        }

  puts("---------------------- PROCESSING DATA ");
  palavra = 0;

  while (getline2(line, TAM, fp))
    {
      i = 0;                         // points to the line start
      search_word(line, &i, wd);     // wd is the pointer to the next token

      if (wd[0]=='0' &&  wd[1]=='x' )
		  // the program line starts with "0x" ==> valid address
           {
             if (DEBUG) printf("\nEndereco de dados %10s\n", wd);

             /// Every word has the format	0 x 1 2 3 4 5 6 7 8
             /// indexes            		0 1 2 3 4 5 6 7 8 9 10

             wd[10] = '\0';                     //  puts a newline char after the last digit
             sscanf(wd+6, "%X", &offset);       //	convert to int from the 4th char on
             offset /= 4;                       //	divide by 4 to obtain word addressing

             palavra = offset / 32;             // The address division gives the memory index

             if( palavra > 63 ) continue;       // Avoids that matrix incurs in overflow

             offset -= palavra*32; // The remainder

             //	wd indices:   2 3 4 5 6 7 8 9     example: 12345678

             while ( search_word(line, &i, wd))     // reads the rest of the line
                if (wd[0]=='0' &&  wd[1]=='x')
                  {  if (DEBUG) printf("p%d o%d %s\n", palavra, offset, wd);
                     ram3[palavra][63-offset*2-1] = wd[2];       // if offset=0   62
                     ram3[palavra][63-offset*2]   = wd[3];       //               63
                     ram2[palavra][63-offset*2-1] = wd[4];
                     ram2[palavra][63-offset*2]   = wd[5];
                     ram1[palavra][63-offset*2-1] = wd[6];
                     ram1[palavra][63-offset*2]   = wd[7];
                     ram0[palavra][63-offset*2-1] = wd[8];
                     ram0[palavra][63-offset*2]   = wd[9];
                     offset += 1;
                  }
            }
     }
   fclose(fp);

    puts("---------------------- Generating the VHDL file memory.vhd ");


   fprintf(out,"--\n--  Automatically Generated - Fernando Moraes & Ney Calazans");
   fprintf(out,"\n--  Generation date: %s--\n\n", ctime(&clock) );
   fprintf(out,"--------------------------------------------------------------------------\n");
   fprintf(out,"--  Program Memory: 512 32-bit words   (2KBytes, 16Kbits, 1 BRAM)\n");
   fprintf(out,"--------------------------------------------------------------------------\n");
   fprintf(out,"library IEEE;\n");
   fprintf(out,"use IEEE.Std_Logic_1164.all;\n");
   fprintf(out,"library UNISIM;\n");
   fprintf(out,"use UNISIM.vcomponents.all; \n");
   fprintf(out,"\n");
   fprintf(out,"entity program_memory is\n");
   fprintf(out,"    port( clock: in std_logic;\n");
   fprintf(out,"          address: in std_logic_vector(8 downto 0);\n");
   fprintf(out,"          instruction: out std_logic_vector(31 downto 0));\n");
   fprintf(out,"end program_memory;\n");
   fprintf(out,"\n");
   fprintf(out,"architecture program_memory of program_memory is     \n");
   fprintf(out,"begin\n");
   fprintf(out,"           \n");
   fprintf(out,"   programa : RAMB16_S36\n");
   fprintf(out,"   generic map (\n");
   for(i=0 ; i<16; i++)
         fprintf(out,"        INIT_0%X => X\"%s\",\n", i, memoria_dados[i]);
   for(i=16 ; i<63; i++)
         fprintf(out,"        INIT_%X => X\"%s\",\n", i, memoria_dados[i]);
   fprintf(out,"        INIT_%X => X\"%s\")\n", i, memoria_dados[i]);
   fprintf(out,"   port map (\n");
   fprintf(out,"            CLK     => clock,\n");
   fprintf(out,"            ADDR    => address,\n");
   fprintf(out,"            EN      => '1',\n");
   fprintf(out,"            WE      => '0',\n");
   fprintf(out,"            DI      => x\"00000000\",\n");
   fprintf(out,"            DIP     => x\"0\",\n");
   fprintf(out,"            DO      => instruction,\n");
   fprintf(out,"            SSR     => '0'\n");
   fprintf(out,"            );\n");
   fprintf(out,"\n");
   fprintf(out,"end program_memory;\n");




   fprintf(out,"\n\n\n--------------------------------------------------------------------------\n");
   fprintf(out,"--  Data memory - 4 blocks of 2048 8-bit words, interleaved (8KBytes, 64Kbits, 4 BRAMs) \n");
   fprintf(out,"--\n");
   fprintf(out,"--  Example: Assuming a data memory with contents:  00 00 00 00\n");
   fprintf(out,"--                                                  10 00 00 AA\n");
   fprintf(out,"--                                                  20 00 BB 00\n");
   fprintf(out,"--                                                  30 CC 00 00\n");
   fprintf(out,"--\n");
   fprintf(out,"--  The data memory first lines are filled as:\n");
   fprintf(out,"--\n");
   fprintf(out,"-- mem 3-  INIT_00 => X\"0000000000000000000000000000000000000000000000000000000030201000\",\n");
   fprintf(out,"-- mem 2-  INIT_00 => X\"00000000000000000000000000000000000000000000000000000000CC000000\",\n");
   fprintf(out,"-- mem 1-  INIT_00 => X\"0000000000000000000000000000000000000000000000000000000000BB0000\",\n");
   fprintf(out,"-- mem 0-  INIT_00 => X\"000000000000000000000000000000000000000000000000000000000000AA00\",\n");
   fprintf(out,"--\n");
   fprintf(out,"--------------------------------------------------------------------------\n");
   fprintf(out,"library IEEE;\n");
   fprintf(out,"use IEEE.Std_Logic_1164.all;\n");
   fprintf(out,"library UNISIM;\n");
   fprintf(out,"use UNISIM.vcomponents.all; \n");
   fprintf(out,"\n");
   fprintf(out,"entity data_memory is\n");
   fprintf(out,"    port( clock, ce, -- For now, ce is not at all used in the data memory,\n");
   fprintf(out,"    	             -- it is stuck at '1'.\n");
   fprintf(out,"          we, bw: in std_logic;\n");
   fprintf(out,"          address: in std_logic_vector(10 downto 0);    -- 11 bits - 2048 addressable words\n");
   fprintf(out,"          byte_choice: in std_logic_vector(1 downto 0); -- 2 bits  - 1 of 4 addressable bytes\n");
   fprintf(out,"          data_in: in std_logic_vector(31 downto 0);\n");
   fprintf(out,"          data_out: out std_logic_vector(31 downto 0));\n");
   fprintf(out,"end data_memory;\n");
   fprintf(out,"\n");
   fprintf(out,"architecture data_memory of data_memory is \n");
   fprintf(out,"    signal we3, we2, we1, we0 : std_logic;\n");
   fprintf(out,"    signal d_in_mem3, d_in_mem2, d_in_mem1, d_in_mem0 : std_logic_vector(7 downto 0);\n");
   fprintf(out,"begin\n");
   fprintf(out,"\n");
   fprintf(out,"    we3 <= '1' when (we='0' and ((bw='0') or (bw='1' and byte_choice=\"11\"))) else '0';\n");
   fprintf(out,"    we2 <= '1' when (we='0' and ((bw='0') or (bw='1' and byte_choice=\"10\"))) else '0';\n");
   fprintf(out,"    we1 <= '1' when (we='0' and ((bw='0') or (bw='1' and byte_choice=\"01\"))) else '0';\n");
   fprintf(out,"    we0 <= '1' when (we='0' and ((bw='0') or (bw='1' and byte_choice=\"00\"))) else '0';\n");
   fprintf(out,"    d_in_mem3 <= data_in(7 downto 0) when bw='1' else data_in(31 downto 24);\n");
   fprintf(out,"    d_in_mem2 <= data_in(7 downto 0) when bw='1' else data_in(23 downto 16);\n");
   fprintf(out,"    d_in_mem1 <= data_in(7 downto 0) when bw='1' else data_in(15 downto 8);\n");
   fprintf(out,"    d_in_mem0 <= data_in(7 downto 0);\n");

   fprintf(out,"\n");
   fprintf(out,"   ----------------------------------------------------------------------------------\n");
   fprintf(out,"   -- bytes 31 a 24\n");
   fprintf(out,"   ----------------------------------------------------------------------------------\n");
   fprintf(out,"    mem_3 : RAMB16_S9\n");
   fprintf(out,"         generic map (\n");
   for(i=0 ; i<16; i++)
         fprintf(out,"        INIT_0%X => X\"%s\",\n", i, ram3[i]);
   for(i=16 ; i<63; i++)
         fprintf(out,"        INIT_%X => X\"%s\",\n", i, ram3[i]);
   fprintf(out,"        INIT_%X => X\"%s\")\n", i, ram3[i]);
   fprintf(out,"  port map (\n");
   fprintf(out,"         CLK     => clock,\n");
   fprintf(out,"         ADDR    => address,\n");
   fprintf(out,"         EN      => '1',\n");
   fprintf(out,"         WE      => we3,\n");
   fprintf(out,"         DI      => d_in_mem3,\n");
   fprintf(out,"         DIP     => \"0\",\n");
   fprintf(out,"         DO      => data_out(31 downto 24),\n");
   fprintf(out,"         SSR     => '0'\n");
   fprintf(out,"         );\n");


   fprintf(out,"\n");
   fprintf(out,"   ----------------------------------------------------------------------------------\n");
   fprintf(out,"   -- bytes 23 a 16\n");
   fprintf(out,"   ----------------------------------------------------------------------------------\n");
   fprintf(out,"    mem_2 : RAMB16_S9\n");
   fprintf(out,"         generic map (\n");
   for(i=0 ; i<16; i++)
         fprintf(out,"        INIT_0%X => X\"%s\",\n", i, ram2[i]);
   for(i=16 ; i<63; i++)
         fprintf(out,"        INIT_%X => X\"%s\",\n", i, ram2[i]);
   fprintf(out,"        INIT_%X => X\"%s\")\n", i, ram2[i]);
   fprintf(out,"  port map (\n");
   fprintf(out,"         CLK     => clock,\n");
   fprintf(out,"         ADDR    => address,\n");
   fprintf(out,"         EN      => '1',\n");
   fprintf(out,"         WE      => we2,\n");
   fprintf(out,"         DI      => d_in_mem2,\n");
   fprintf(out,"         DIP     => \"0\",\n");
   fprintf(out,"         DO      => data_out(23 downto 16),\n");
   fprintf(out,"         SSR     => '0'\n");
   fprintf(out,"         );\n");


   fprintf(out,"\n");
   fprintf(out,"   ----------------------------------------------------------------------------------\n");
   fprintf(out,"   -- bytes 15 a 8\n");
   fprintf(out,"   ----------------------------------------------------------------------------------\n");
   fprintf(out,"    mem_1 : RAMB16_S9\n");
   fprintf(out,"         generic map (\n");
   for(i=0 ; i<16; i++)
         fprintf(out,"        INIT_0%X => X\"%s\",\n", i, ram1[i]);
   for(i=16 ; i<63; i++)
         fprintf(out,"        INIT_%X => X\"%s\",\n", i, ram1[i]);
   fprintf(out,"        INIT_%X => X\"%s\")\n", i, ram1[i]);
   fprintf(out,"  port map (\n");
   fprintf(out,"         CLK     => clock,\n");
   fprintf(out,"         ADDR    => address,\n");
   fprintf(out,"         EN      => '1',\n");
   fprintf(out,"         WE      => we1,\n");
   fprintf(out,"         DI      => d_in_mem1,\n");
   fprintf(out,"         DIP     => \"0\",\n");
   fprintf(out,"         DO      => data_out(15 downto 8),\n");
   fprintf(out,"         SSR     => '0'\n");
   fprintf(out,"         );\n");



   fprintf(out,"\n");
   fprintf(out,"   ----------------------------------------------------------------------------------\n");
   fprintf(out,"   -- bytes 7 a 0\n");
   fprintf(out,"   ----------------------------------------------------------------------------------\n");
   fprintf(out,"    mem_0 : RAMB16_S9\n");
   fprintf(out,"         generic map (\n");
   for(i=0 ; i<16; i++)
         fprintf(out,"        INIT_0%X => X\"%s\",\n", i, ram0[i]);
   for(i=16 ; i<63; i++)
         fprintf(out,"        INIT_%X => X\"%s\",\n", i, ram0[i]);
   fprintf(out,"        INIT_%X => X\"%s\")\n", i, ram0[i]);
   fprintf(out,"  port map (\n");
   fprintf(out,"         CLK     => clock,\n");
   fprintf(out,"         ADDR    => address,\n");
   fprintf(out,"         EN      => '1',\n");
   fprintf(out,"         WE      => we0,\n");
   fprintf(out,"         DI      => d_in_mem0,\n");
   fprintf(out,"         DIP     => \"0\",\n");
   fprintf(out,"         DO      => data_out(7 downto 0),\n");
   fprintf(out,"         SSR     => '0'\n");
   fprintf(out,"         );\n");

   fprintf(out,"\n");
   fprintf(out,"end data_memory;\n");

   fclose(out);

   return(1);

}
