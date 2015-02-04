
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// *** Compteur d'instruction ***
int cptInstrution = 0;


/**********************************************************
** Codage d'une instruction (32 bits)
***********************************************************/

typedef struct {
	unsigned OP: 10;  /* code operation (10 bits)  */
	unsigned i:   3;  /* nu 1er registre (3 bits)  */
	unsigned j:   3;  /* nu 2eme registre (3 bits) */
	short    ARG;     /* argument (16 bits)        */
} INST;


/**********************************************************
** definition de la memoire simulee
***********************************************************/

typedef int WORD;  /* un mot est un entier 32 bits  */

WORD mem[128];     /* memoire                       */


/**********************************************************
** Codes associes aux instructions
***********************************************************/

#define INST_ADD	  (0)
#define INST_SUB	  (1)
#define INST_CMP	  (2)
#define INST_IFGT   (3)
#define INST_NOP    (4)
#define INST_JUMP   (5)
#define INST_HALT   (6)
#define INST_SYSC   (7)
#define INST_LOAD   (8)

/**********************************************************
** Codes associes aux instructions
***********************************************************/



/**********************************************************
** Placer une instruction en memoire
***********************************************************/

void make_inst(int adr, unsigned code, unsigned i, unsigned j, short arg) {
	union { WORD word; INST fields; } inst;
	inst.fields.OP  = code;
	inst.fields.i   = i;
	inst.fields.j   = j;
	inst.fields.ARG = arg;
	mem[adr] = inst.word;
}


/**********************************************************
** Codes associes aux interruptions
***********************************************************/

#define INT_NONE  (0)
#define INT_INIT	(1)
#define INT_SEGV	(2)
#define INT_INST	(3)
#define INT_TRACE	(4)
#define INT_CLOCK (5)

// *** SYSC interruption *** 
#define SYSC_EXIT 			(100)
#define SYSC_PUTI			 	(101)
#define SYSC_NEW_THREAD	(102)

/**********************************************************
** Le Mot d'Etat du Processeur (PSW)
***********************************************************/

typedef struct PSW {    /* Processor Status Word */
	WORD PC;        /* Program Counter */
	WORD SB;        /* Segment Base */
	WORD SS;        /* Segment Size */
	WORD IN;        /* Interrupt number */
	WORD DR[8];     /* Data Registers */
	WORD AC;        /* Accumulateur */
	INST RI;        /* Registre instruction */
} PSW;

/*********************************************************
** Représentation de l'ensemble des processus
**********************************************************/
#define MAX_PROCESS (20)

#define EMPTY		(0)
#define READY		(1)

struct
{
	PSW cpu;
	int state;
} process[ MAX_PROCESS ];

int current_process = -1;

/**********************************************************
** Simulation de la CPU (mode utilisateur)
***********************************************************/

/* instruction d'addition */
PSW cpu_ADD(PSW m) {
	m.AC = m.DR[m.RI.i] += (m.DR[m.RI.j] + m.RI.ARG);
	m.PC += 1;
	return m;
}

/* instruction de soustraction */
PSW cpu_SUB(PSW m) {
	m.AC = m.DR[m.RI.i] -= (m.DR[m.RI.j] + m.RI.ARG);
	m.PC += 1;
	return m;
}

/* instruction de comparaison */
PSW cpu_CMP(PSW m) {
	m.AC = (m.DR[m.RI.i] - (m.DR[m.RI.j] + m.RI.ARG));
	m.PC += 1;
	return m;
}

PSW cpu_LOAD( PSW m )
{
	m.AC = m.DR[ m.RI.j ] + m.RI.ARG;
	if( m.AC < 0 || m.AC > m.SS )
	{
		m.IN = INT_SEGV;
	}
	else
	{
		m.AC = mem[ m.SB + m.AC ];
		m.DR[ m.RI.i ] = m.AC;
		m.PC += 1;
	}

	return m;
}

/* Simulation de la CPU */
PSW cpu(PSW m) {
	union { WORD word; INST in; } inst;

	/*** lecture et decodage de l'instruction ***/
	if (m.PC < 0 || m.PC >= m.SS) {
		m.IN = INT_SEGV;
		return (m);
	}
	inst.word = mem[m.PC + m.SB];
	m.RI = inst.in;
	/*** execution de l'instruction ***/
	switch (m.RI.OP) {
    
    case INST_ADD:
      m = cpu_ADD(m);
      break;
    
    case INST_SUB:
      m = cpu_SUB(m);
      break;
    
    case INST_CMP:
      m = cpu_CMP(m);
      break;
    
    case INST_IFGT:
      if( m.AC > 0 )
        m.PC = m.RI.ARG;
      else
      ++m.PC;
      break;

    case INST_NOP:
      ++m.PC;
      break;

    case INST_JUMP:
      m.PC = m.RI.ARG;
      break;

    case INST_HALT:
      break;

    case INST_SYSC:
      m.IN = m.RI.ARG;
			++m.PC;
    break;

    case INST_LOAD:
			cpu_LOAD( m );
      break;



    default:
      /*** interruption instruction inconnue ***/
      m.IN = INT_INST;
      return (m);
	}

	/*** interruption apres chaque instruction ***/
  if( (++cptInstrution & 0x3) == 0x3 && m.IN == INT_NONE )
    m.IN = INT_CLOCK;

  
  return m;
}


/**********************************************************
** Demarrage du systeme
***********************************************************/

PSW systeme_init(void) {
	PSW cpu;

	current_process = -1;
	for( int i = 0; i < MAX_PROCESS; ++i )
		process[ i ].state = EMPTY;

	printf("Booting.\n");
	/*** creation d'un programme ***/
	// make_inst(0, INST_SUB, 2, 2, -1000); /* R2 -= R2-1000 */
	// make_inst(1, INST_ADD, 1, 2, 500);   /* R1 += R2+500 */
	// make_inst(2, INST_ADD, 0, 2, 200);   /* R0 += R2+200 */
	// make_inst(3, INST_ADD, 0, 1, 100);   /* R0 += R1+100 */

  make_inst( 0, INST_SUB, 1, 1, 0 );
  make_inst( 1, INST_SUB, 2, 2, -1000 );
  make_inst( 2, INST_SUB, 3, 3, -10 );
  make_inst( 3, INST_SYSC, 1, 0, SYSC_PUTI );
  make_inst( 4, INST_SYSC, 2, 0, SYSC_PUTI );
  make_inst( 5, INST_SYSC, 3, 0, SYSC_PUTI );
  make_inst( 6, INST_CMP, 1, 2, 0 );
  make_inst( 7, INST_IFGT, 0, 0, 14 );
  make_inst( 8, INST_NOP, 0, 0, 0 );
  make_inst( 9, INST_NOP, 0, 0, 0 );
  make_inst( 10, INST_NOP, 0, 0, 0 );
  make_inst( 11, INST_ADD, 1, 3, 0 );
  make_inst( 12, INST_SYSC, 1, 0, SYSC_PUTI );
  make_inst( 13, INST_JUMP, 0, 0, 6 );
  make_inst( 14, INST_HALT, 0, 0, 0 );


	/*** valeur initiale du PSW ***/
	memset (&cpu, 0, sizeof(cpu));
	cpu.PC = 0;
	cpu.SB = 0;
	cpu.SS = 20;

	process[0].cpu = cpu;
	process[0].state = READY;
	process[1].cpu = cpu;
	process[1].state = READY;
	current_process = 0;

	return cpu;
}


/**********************************************************
** Simulation du systeme (mode systeme)
***********************************************************/

PSW systeme(PSW m) {
	switch (m.IN) {
		case INT_INIT:
			return (systeme_init());

		case INT_SEGV:
      printf( "%d - INT_SEGV received...\n", m.IN );
      exit( INT_SEGV );
			break;
		
    case INT_TRACE:
      printf( "%d - INT_TRACE received...\nPC : %d\nDR : \n", m.IN, m.PC );
      for( int i = 0; i < 8; ++i )
        printf( "\tDR[%d] : %d\n", i, m.DR[i] );
			break;
		
    case INT_INST:
      printf( "%d - INT_INST received...\n", m.IN );
      exit( INT_INST );
			break;


    case INT_CLOCK:
			if( current_process != -1 )
			{
				process[ current_process ].cpu = m;
				process[ current_process ].state = READY;
			}

			do
			{
				current_process = (current_process + 1) % MAX_PROCESS;
			}
			while( process[ current_process ].state != READY );
			
			return process[ current_process ].cpu;
      break;

    // *** SYSC interruption
		case SYSC_PUTI:
			printf( "SYSC_PUTI : %d\n", m.DR[ m.RI.i ] );
			break;

    case SYSC_EXIT:
      exit( 0 );

		case SYSC_NEW_THREAD:
			int tmpId = -1;
			do
			{
				tmpId = current_process + 1 % MAX_PROCESS;
			}
			while( process[ tmpId ].state = EMPTY );
			m.DR

		default:
			break;
	}


  //system( "sleep 0.05" );
  m.IN = INT_NONE;
	return m;
}


/**********************************************************
** fonction principale
** (ce code ne doit pas etre modifie !)
***********************************************************/

int main(void) {
	PSW mep;
	
	mep.IN = INT_INIT; /* interruption INIT */	
	while (1) {
		mep = systeme(mep);
		mep = cpu(mep);
	}
	
	return (EXIT_SUCCESS);
}

