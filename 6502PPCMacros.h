/***********************************************************//**                                                       **//**              6   5   0   2   P   P   C                **//**                                                       **//***********************************************************/#define zeroPage1800 r13#define zeroPage1000 r14#define zeroPage800  r15#define zeroPage     r16#define cyclesBack   r17#define M6502R       r18#define jumpTable    r19#define afterCLI     r20#define rPC          r21#define rA           r22#define rX           r23#define rY           r24#define rS           r25#define rP           r26#define rZ           r27#define cycles       r28#define workAddr     r29#define pageTable    r30#define stackPage    r31// #define USE_DECIMAL_FLAG // <-- NES does not have BCD ADC/SBC// assumptions// pages are 8K// pageTable is Page(RTOC)// zeroPage contains 0(pageTable)// stackPage contains 0(pageTable) + 0x100// rPC points to the first byte of the current instruction// rA, rX, rY, rS and rP must be masked whenever modified// rP will probably not contain valid Z flag// rZ stores the Z flag in bit 0x20// notes// bit 0 of PPC CR is negative flag// bit 2 of PPC CR is zero flag// bit 3 of PPC CR is overflow flag// reads directly out of page table#define Op6502(adr, dst)                                                              \	rlwinm   r6, adr, 21, 27, 29;    /* get bits 16-18 into bits 27-29, strip rest */ \	lwzx     r7, pageTable, r6;      /* get proper page */                            \	rlwinm   r6, adr, 0, 19, 31;     /* strip address high part */                    \	lbzx     dst, r6, r7;            /* return the proper byte from that page */// reads 16-bit value from PC into dst#define Load16FromPC( dst, label )                   \	rlwinm r7, rPC, 0, 19, 31;                       \	rlwinm r6, rPC, 21, 27, 29;                      \	cmpwi  r7, 0x1FFF;                               \	lwzx   r8, pageTable, r6;                        \	addi   rPC, rPC, 2;                              \	bne+   label##useBR;                             \	lbz    dst, 0x1FFF(r8);                          \	addi   r6, r6, 4;                                \	rlwinm r6, r6, 0, 27, 29;                        \	lwzx   r8, pageTable, r6;                        \	lbz    r7, 0(r8);                                \	rlwimi dst, r7, 8, 16, 23;                       \	b      label##load16out;                         \label##useBR:                                        \	lhbrx  dst, r8, r7;                              \label##load16out:// reads a 16-bit value from first 256 bytes of RAM// TODO: if this reads from byte 255, should 2nd byte wrap around to 0?#define Load16FromZP( src, dst )           \	rlwinm r5, src, 0, 24, 31;             \	lhbrx  dst, zeroPage, r5;// pops into reg#define Pop(reg)                     \	addi   rS, rS, 1;                \	rlwinm rS, rS, 0, 24, 31;        \	lbzx   reg, stackPage, rS;// pushes from reg#define Push(reg)                    \	stbx   reg, stackPage, rS;       \	addi   r11, rS, 0x100;           \	stbx   reg, zeroPage800, r11;    \    addi   rS, rS, -1;               \	stbx   reg, zeroPage1000, r11;   \	rlwinm rS, rS, 0, 24, 31;        \	stbx   reg, zeroPage1800, r11;// pops the PC#define PopPC                      \	Pop( rPC )                     \	Pop( r5 )                      \	rlwimi rPC, r5, 8, 16, 23;// pushes the PC#define PushPC( offset )           \	addi   r6, rPC, offset;        \	rlwinm r5, r6, 24, 24, 31;     \	Push( r5 )                     \	Push( r6 )// Sets rZN#define SetZN( reg )               \	cntlzw rZ, reg;                \	rlwimi rP, reg, 0, 24, 24;	// packs rZ into rP's Z_FLAG, N_FLAG#define PackZ                                    \	rlwimi rP, rZ, 28, 30, 30; /* get Z_FLAG */// unpacks rP's Z_FLAG into rZ#define UnpackZ					                 \	rlwinm rZ, rP, 4, 26, 26;  /* get Z_FLAG */	// sets flag bits for comparison operation#define Compare( reg1, reg2 )                             \	sub     r9, reg1, reg2;                               \	rlwimi  rP, r9, 24, 31, 31;  /* set C_FLAG if neg */  \	rlwinm  r9, r9, 0, 24, 31;   /* mask carry */         \	xori    rP, rP, C_FLAG;      /* invert C_FLAG */      \	SetZN( r9 )                  /* get Z_FLAG, N_FLAG */// sets flag bits for a BIT operation#define BitwiseCompare( reg )                            \	and    r9, reg, rA;                                  \	rlwimi rP, reg, 0, 24, 25;  /* set N_FLAG, V_FLAG */ \	cntlzw rZ, r9;              /* set rZ */ // does a DEC and sets flags#define Decrement( src, dst )   \	addi   dst, src, -1;        \	rlwinm dst, dst, 0, 24, 31; \	SetZN( dst )	// does an INC and sets flags#define Increment( src, dst )   \	addi   dst, src,  1;        \	rlwinm dst, dst, 0, 24, 31; \	SetZN( dst )                       	// does an XOR with acc and sets flags#define Xor( reg )      \	xor    rA, rA, reg; \	SetZN( rA )// does an OR with acc and sets flags#define Or( reg )       \	or     rA, rA, reg; \	SetZN( rA )// does an AND with acc and sets flags#define And( reg )      \	and    rA, rA, reg; \	SetZN( rA )// does an ADC with acc (add with carry) and sets appropriate flags// NOTE: flag handling for decimal mode is significantly different than SBC--//       this behaviour is documented as being correct, however. Flags for//       ADC and SBC are detailed in John West's 64doc.#ifdef USE_DECIMAL_FLAG#define AddWithCarry( value, label )    \	rlwinm. r9, rP, 0, 28, 28; /* check D_FLAG */ \	add    r10, rA, value;              \	rlwinm r11, rP, 0, 31, 31;          \	add    r10, r10, r11;               \	rlwinm r9, r10, 0, 24, 31;          \	cntlzw rZ, r9;                   /* set rZ */ \	beq+   label##noD;                  \	rlwinm r9, rA, 0, 28, 31;        /* get rA's low nybble */   \	rlwinm r10, value, 0, 28, 31;    /* get value's low nybble */   \	add    r8, r9, r11;              /* add C_FLAG + rA */  \	add    r8, r8, r10;              /* add C_FLAG + rA + value */   \	rlwinm r9, rA, 0, 24, 27;        /* get rA's high nybble */   \	cmpwi  r8, 9;                    /* test low nybble sum */   \	rlwinm r10, value, 0, 24, 27;    /* get value's high nybble */   \	add    r9, r9, r10;              /* add rA + value */   \	ble+   label##over;                 \	addi   r8, r8, 6;                /* add 6 to low nybble */   \	addi   r9, r9, 0x10;             /* add 0x10 to high nybble */  \label##over:                            \	eqv    r10, rA, value;              \	cmpwi  r9, 0x90;                 /* test high nybble sum */   \	xor    r11, rA, r9;                 \	rlwimi rP, r9, 0, 24, 24;        /* set N_FLAG */ \	and    r10, r10, r11;               \	rlwimi rP, r10, 31, 25, 25;      /* set V_FLAG */ \	ble+   label##over2;                \	addi   r9, r9, 0x60;             /* add 0x60 to high nybble */   \label##over2:						 	\	rlwimi r8, r9, 0, 24, 27;        /* stick nybbles together */   \	rlwimi rP, r9, 24, 31, 31;       /* set C_FLAG */   \	mr     rA, r8;                   /* move completed value into rA */   \	b      label##out;                  \label##noD:                             \	rlwimi rP, r10, 24, 31, 31; /* set C_FLAG */ \	rlwimi rP, r10, 0, 24, 24;  /* set N_FLAG */ \	eqv    r10, rA, value;              \	xor    r11, rA, r9;                 \	mr     rA, r9;                      \	and    r10, r10, r11;               \	rlwimi rP, r10, 31, 25, 25; /* set V_FLAG */ \label##out:#else#define AddWithCarry( value, label )    \	add    r10, rA, value;              \	rlwinm r11, rP, 0, 31, 31;          \	add    r10, r10, r11;               \	rlwinm r9, r10, 0, 24, 31;          \	cntlzw rZ, r9;                   /* set rZ */ \	rlwimi rP, r10, 24, 31, 31; /* set C_FLAG */ \	rlwimi rP, r10, 0, 24, 24;  /* set N_FLAG */ \	eqv    r10, rA, value;              \	xor    r11, rA, r9;                 \	mr     rA, r9;                      \	and    r10, r10, r11;               \	rlwimi rP, r10, 31, 25, 25; /* set V_FLAG */#endif// does an SBC with acc (subtract with carry) and sets appropriate flags// NOTE: flag handling for decimal mode is significantly different than ADC--//       this behaviour is documented as being correct, however. Flags for//       ADC and SBC are detailed in John West's 64doc.#ifdef USE_DECIMAL_FLAG#define SubtractWithCarry( value, label )  \	rlwinm. r9, rP, 0, 28, 28;     /* check D_FLAG */ \	rlwinm r11, rP, 0, 31, 31;     /* get carry flag */ \	sub    r10, rA, value;         /* get acc minus value */ \	xori   r8, r11, 1;             /* invert carry (keep in r8) */ \	sub    r10, r10, r8;           /* get acc minus value minus ~C_FLAG */ \	rlwinm r9, r10, 0, 24, 31;     /* strip high bits from result */ \	xori   r11, r10, 0x100;        /* invert resultant carry */ \	cntlzw rZ, r9;                 /* set rZ */ \	rlwimi rP, r11, 24, 31, 31;    /* set C_FLAG */ \	xor    r10, rA, value;              \	xor    r11, rA, r9;                 \	rlwimi rP, r9, 0, 24, 24;      /* set N_FLAG */ \	and    r10, r10, r11;               \	rlwimi rP, r10, 31, 25, 25;    /* set V_FLAG */ \	beq+   label##noD;                  \	rlwinm r9, rA, 0, 28, 31;      /* get rA's low nybble */   \	rlwinm r10, value, 0, 28, 31;  /* get value's low nybble */   \	sub    r8, r9, r8;             /* rA's low nybble minus inverted carry */   \	sub    r8, r8, r10;            /* subtract rA - ~C_FLAG - value */    \	andi.  r10, r8, 0x10;          /* test 0x10 bit in r8 */   \	rlwinm r9, rA, 0, 24, 27;      /* mask high nybble of rA */   \	rlwinm r10, value, 0, 24, 27;  /* mask high nybble of value */   \	sub    r9, r9, r10;            /* subtract them */   \	beq+   label##over;            /* if r8 & 0x10, then: */   \	subi   r8, r8, 6;              /* decrement r8 down */   \	subi   r9, r9, 0x10;           /* decrement r9 more */   \label##over:                            \	andi.  r10, r9, 0x100;         /* test 0x100 bit in r9 */   \	beq+   label##over2;           /* if r9 & 0x10, then: */   \	subi   r9, r9, 0x60;           /* decrement r9 */   \label##over2:                           \	rlwimi r9, r8, 0, 28, 31;      /* load low nybble of r8 into r9 */ \label##noD:                             \	rlwinm rA, r9, 0, 24, 31;      /* set and mask acc */	#else#define SubtractWithCarry( value, label )  \	rlwinm r11, rP, 0, 31, 31;     /* get carry flag */ \	sub    r10, rA, value;         /* get acc minus value */ \	xori   r8, r11, 1;             /* invert carry (keep in r8) */ \	sub    r10, r10, r8;           /* get acc minus value minus ~C_FLAG */ \	rlwinm r9, r10, 0, 24, 31;     /* strip high bits from result */ \	xori   r11, r10, 0x100;        /* invert resultant carry */ \	cntlzw rZ, r9;                 /* set rZ */ \	rlwimi rP, r11, 24, 31, 31;    /* set C_FLAG */ \	xor    r10, rA, value;              \	xor    r11, rA, r9;                 \	rlwimi rP, r9, 0, 24, 24;      /* set N_FLAG */ \	and    r10, r10, r11;               \	rlwimi rP, r10, 31, 25, 25;    /* set V_FLAG */ \	rlwinm rA, r9, 0, 24, 31;      /* set and mask acc */	#endif// does an move and sets ZN#define Transfer( src, dst )   \	mr    dst, src;            \	SetZN( src )// does a move#define TransferWithoutFlag( src, dst ) \	mr    dst, src;// does a ROR, setting up C_FLAG, N_FLAG, and Z_FLAG// Caution: dst and src can be the same register!#define RotateRight( src, dst )                                  \	rlwimi src, rP, 8, 23, 23;     /* appends C_FLAG onto src */ \	rlwimi rP, src, 0, 31, 31;     /* fill C_FLAG */             \	rlwinm dst, src, 31, 24, 31;   /* rotate & mask into dst */  \	SetZN( dst )                   /* set up Z_FLAG, N_FLAG */// does a ROL, setting up C_FLAG, N_FLAG, and Z_FLAG// Caution: dst and src can be the same register!#define RotateLeft( src, dst )                                    \	rlwimi src, rP, 31, 0, 0;      /* appends C_FLAG below src */ \	rlwimi rP, src, 25, 31, 31;    /* fill C_FLAG */              \	rlwinm dst, src, 1, 24, 31;    /* rotate & mask into dst */   \	SetZN( dst )                   /* set up Z_FLAG, N_FLAG */// does an LSR, setting up C_FLAG, N_FLAG, and Z_FLAG// Caution: dst and src can be the same register!#define LogicalShiftRight( src, dst )                                                \	rlwimi rP, src, 0, 31, 31;    /* set the C_FLAG (bit 31) to the low bit of r3 */ \	rlwinm dst, src, 31, 25, 31;  /* shift r3 right one  */                          \	SetZN( dst )                  /* set up Z_FLAG, N_FLAG */// does an ASL, setting up C_FLAG, N_FLAG, and Z_FLAG// Caution: dst and src can be the same register!#define ArithmeticShiftLeft( src, dst )                                               \	rlwimi rP, src, 25, 31, 31;  /* set the C_FLAG (bit 31) to the high bit of src */ \	rlwinm dst, src, 1, 24, 30;  /* shift left one */                                 \	SetZN( dst )                 /* set up Z_FLAG, N_FLAG */// branch if#define BranchIf( reg, mask, jump, label )                \	andi.  r9, reg, mask;                                 \	jump   label##noBranch;                               \	Op6502( rPC, r10 )                                    \	extsb  r10, r10;                                      \	subi   cycles, cycles, 1;                             \	add    rPC, rPC, r10;                                 \label##noBranch: 										  \	addi   rPC, rPC, 1;// branches if any bits are on #define BranchIfAnyOn( reg, mask, label )   \	BranchIf( reg, mask, beq, label )	// branches if all bits are off#define BranchIfAllOff( reg, mask, label )    \	BranchIf( reg, mask, bne, label )// starts a page zero read opcode, memory just read is placed into r3#define ZeroPageRead               \	Op6502( rPC, workAddr )        \	addi   rPC, rPC, 1;            \	lbzx   r3, zeroPage, workAddr; // does zero-page addressed write to memory#define ZeroPageWrite( reg )             \	Op6502( rPC, workAddr )              \	stbx   reg, zeroPage, workAddr;      \	stbx   reg, zeroPage800, workAddr;   \	addi   rPC, rPC, 1;                  \	stbx   reg, zeroPage1000, workAddr;  \	stbx   reg, zeroPage1800, workAddr; // starts a page zero modify opcode#define ZeroPageModifyStart        \	Op6502( rPC, workAddr )        \	addi   rPC, rPC, 1;            \	lbzx   r3, zeroPage, workAddr;// ends a page zero modify opcode (writes back change)// value to write must be stored in r4#define ZeroPageModifyEnd              \	stbx   r4, zeroPage, workAddr;     \	stbx   r4, zeroPage800, workAddr;  \	stbx   r4, zeroPage1000, workAddr; \	stbx   r4, zeroPage1800, workAddr;// starts a page zero x-indexed read opcode, memory just read is placed into r3#define ZeroPageIndexRead( reg )          \	Op6502( rPC, workAddr )               \	add    workAddr, workAddr, reg;       \	addi   rPC, rPC, 1;                   \	rlwinm workAddr, workAddr, 0, 24, 31; \	lbzx   r3, zeroPage, workAddr;// does zero-page x-indexed addressed write to memory#define ZeroPageIndexWrite( reg, write )  \	Op6502( rPC, workAddr )               \	add    workAddr, workAddr, reg;       \	addi   rPC, rPC, 1;                   \	rlwinm workAddr, workAddr, 0, 24, 31; \	stbx   write, zeroPage, workAddr;     \	stbx   write, zeroPage800, workAddr;  \	stbx   write, zeroPage1000, workAddr; \	stbx   write, zeroPage1800, workAddr;// starts a page zero x-indexed modify opcode#define ZeroPageIndexModifyStart( reg )   \	Op6502( rPC, workAddr )               \	add    workAddr, workAddr, reg;       \	addi   rPC, rPC, 1;                   \	rlwinm workAddr, workAddr, 0, 24, 31; \	lbzx   r3, zeroPage, workAddr;// ends a page zero x-indexed modify opcode (writes back change)// must have value to write stored in r4!!!!!!!#define ZeroPageIndexModifyEnd( reg )  \	stbx   r4, zeroPage, workAddr;     \	stbx   r4, zeroPage800, workAddr;  \	stbx   r4, zeroPage1000, workAddr; \	stbx   r4, zeroPage1800, workAddr;// starts a absolute read opcode, memory just read is placed into r3#define AbsoluteRead( label )   \	Load16FromPC( r3, label )   \	bl     Rd6502;             // does an absolute mode write#define AbsoluteWrite( reg, label )  \	Load16FromPC( r3, label )        \	mr     r4, reg;                  \	bl     Wr6502;              // starts a absolute modify opcode#define AbsoluteModifyStart( label )           \	Load16FromPC( workAddr, label )            \	mr     r3, workAddr;                       \	bl     Rd6502;                    // ends a absolute modify opcode (writes back change)// must have value to write stored in r4!!!!!!!#define AbsoluteModifyEnd(label) \	mr     r3, workAddr;         \ 	bl     Wr6502;        // starts a absolute read opcode, memory just read is placed into r3#define AbsoluteIndexRead( reg, label )   \	Load16FromPC( r3, label )             \	add    r3, r3, reg;                   \	bl     Rd6502;                     // does an absolute mode write#define AbsoluteIndexWrite( reg, write, label )  \	Load16FromPC( workAddr, label )              \	add    r3, workAddr, reg;                    \	mr     r4, write;                            \	bl     Wr6502;                      // starts a absolute modify opcode#define AbsoluteIndexModifyStart( reg, label )  \	Load16FromPC( workAddr, label )             \	add    r3, workAddr, reg;                   \ 	bl     Rd6502;                    // ends a absolute modify opcode (writes back change)// must have value to write stored in r4!!!!!!!#define AbsoluteIndexModifyEnd( reg, label ) \	add    r3, workAddr, reg;               \ 	bl     Wr6502;                 // starts an immediate mode read#define ImmediateRead( dst ) \	Op6502( rPC, dst )       \	addi   rPC, rPC, 1;// starts an indexed indirect read ($ss, x)#define IndexIndirRead               \	Op6502( rPC, r3 )                \	add    r3, r3, rX;               \	addi   rPC, rPC, 1;              \	Load16FromZP( r3, r3 )           \	bl     Rd6502;            // does an indexed indirect write ($ss, x)#define IndexIndirWrite( reg )       \	Op6502( rPC, r3 )                \	add    r3, r3, rX;               \	addi   rPC, rPC, 1;              \	Load16FromZP( r3, r3 )           \ 	mr     r4, reg;                  \	bl     Wr6502;             // starts an indirect indexed read ($ss), y#define IndirIndexRead            \	Op6502( rPC, r3 )             \	addi   rPC, rPC, 1;           \	Load16FromZP( r3, r3 )        \	add    r3, r3, rY;            \	bl     Rd6502;            	// does an indirect indexed write ($ss), y#define IndirIndexWrite( reg )    \	Op6502( rPC, r3 )             \	addi   rPC, rPC, 1;           \	Load16FromZP( r3, r3 )        \	add    r3, r3, rY;            \	mr     r4, reg;               \	bl     Wr6502;            // finishes an opcode, updates cycles, does next opcode#define EndOpcode( cyc )        \    subic. cycles, cycles, cyc; \    b nextOpcode;