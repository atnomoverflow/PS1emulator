#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <iomanip>
#include <bitset>
#include<tuple>
#include <array>
#include <vector> 
struct RegisterIndex
{
	uint32_t index;
	RegisterIndex()
	{
		index=0;
	}
	RegisterIndex(const RegisterIndex& regi ){index=regi.index;}
	RegisterIndex(uint32_t i)
	{
		index=i;
	}
	uint32_t operator|(const RegisterIndex& regindx)
	{
		return index|regindx.index;
	}	
};
//adress maper
namespace map
{
	const uint32_t REGION_MASK[8]={
		//  KUSEG:   2048MB
		0xffffffff  ,   0xffffffff  ,   0xffffffff  ,   0xffffffff  ,
		//  KSEG0 :512MB
		0x7fffffff  ,
		//  KSEG1 :512MB
		0x1fffffff  ,
		//  KSEG2 :   1024MB
		0xffffffff  ,   0xffffffff }; 
	///mask a cpu address to remove the reigon bits
	uint32_t mask_region(uint32_t addr)
	{
		size_t index=(size_t)(addr>>29);
		return (addr & REGION_MASK[index]);
	}
	// this structure used to create ranges for adresses
	typedef  struct Range
	{
		uint32_t start;
		uint32_t length;
		
		uint32_t contains(uint32_t addr) const
		{
			if(		(addr>=start)	&&	(addr< length+start)	)
			{
				return addr-start+1;
			}
			else
			{
				return (uint32_t)0;
			}
		}

	}MyRange;
	//ranges of adresses
	const MyRange BIOS 			=	{0x1fc00000	,	512*1024	};
	const MyRange MEMORYCONTROL =	{0x1f801000 ,	36			};
	const MyRange RAM_SIZE 		=	{0x1f801060	,	4			};
	const MyRange CACHE_CONTROLE=	{0xfffe0130	,	4			};
	const MyRange RAM			=	{0x00000000	,	2*1024*1024	};
	const MyRange SPU			=	{0x1f801c00 ,   640			};
	const MyRange EXPENTION_2	=	{0x1f802000 ,   66 			};
	
}
///RAM a vertual one cool !!!
struct RAM
{
	std::vector<uint8_t> data;
	RAM()
	{
		for(int i=0;i<(2*1024*1024);i++ )
		{
			data.push_back(0xca);
		}
	}
	uint32_t Load32bit(uint32_t offset)
	{
		uint32_t b0=(uint32_t)(data[	offset			]);
		uint32_t b1=(uint32_t)(data[	offset	+	1	]);
		uint32_t b2=(uint32_t)(data[	offset	+	2	]);
		uint32_t b3=(uint32_t)(data[	offset	+	3	]);
		// ps1 is littel endian :-D
		return	(	b0	|	(b1	<<	8)	|	(b2	<<	16)	|	(b3	<<	24));
	}
	void Store32bit(uint32_t offset,uint32_t val)
	{
		uint8_t b0	=	(uint8_t)val		;
		uint8_t b1	=	(uint8_t)(val>>8)	;
		uint8_t b2	=	(uint8_t)(val>>16)	;
		uint8_t b3	=	(uint8_t)(val>>24)	;
		
		data[	offset			]	=	b0;
		data[	offset	+	1	]	=	b1;
		data[	offset	+	2	]	=	b2;
		data[	offset	+	3	]	=	b3;
	}

};
///BIOS 
struct BIOS
{
	static constexpr uint64_t BIOS_SIZE=512*1024;
	uint8_t data[BIOS_SIZE];
	///initialize the BIOS
	BIOS(){};
	BIOS(const char * path){
		
		    if (FILE *fp = fopen(path, "rb"))
		    {
				
		    	fread(data,1, BIOS_SIZE, fp);
		    	fclose(fp);
		    }
	}
	///fetch 8bit data from bios
	uint8_t Load8bit(uint32_t offset)
	{
		std::cout <<"hello";
		return data[(size_t)offset];
	}
	///fetch 32bit data from BIOS data
	uint32_t Load32bit(uint32_t offset)
	{
		uint32_t b0=data[	offset			];
		uint32_t b1=data[	offset	+	1	];
		uint32_t b2=data[	offset	+	2	];
		uint32_t b3=data[	offset	+	3	];
		// ps1 is littel endian :-D
		return	(	b0	|	(b1	<<	8)	|	(b2	<<	16)	|	(b3	<<	24));
	}
};
struct Interconnect
{
	BIOS* Bios;
	RAM*  Ram;

	Interconnect(){};
	Interconnect(BIOS* B,RAM* R)
	{
		Bios=B;
		Ram=R;
	}
	///Load 8bit
	uint8_t Load8bit(uint32_t addr)
	{
		
		uint32_t abs_addr=map::mask_region(addr);
		if(uint32_t offset=map::BIOS.contains(abs_addr))
		{
			std::cout <<"bios load8bit at address 0x"<<addr;
			return Bios->Load8bit(offset);
			
		}
		std::cout <<"unaligned load8bit at address 0x"<<std::hex<<addr;
		exit(0);
	}
	///Load a 32 bit word into the the addr
	uint32_t Load32bit(uint32_t addr)
	{
		if((addr%4)	!=	0)
		{
			
			exit(0);
		}
		uint32_t abs_adr=map::mask_region(addr);
		if(uint32_t offset= map::BIOS.contains(abs_adr)){
		
			
			return Bios->Load32bit(offset-1);
		} 
		if(uint32_t offset=map::RAM.contains(abs_adr))
		{
			return Ram->Load32bit(offset-1);
		}
		else
		{	
			exit(0);
		}
	}
	///Store 8bit `val` into addr
	void Store8(uint32_t addr,uint8_t)
	{
		uint32_t abs_addr=map::mask_region(addr);
		if(uint32_t offset=map::EXPENTION_2.contains(abs_addr))
		{
			std::cout <<"unaligned write expention 2 register 0x"<<std::hex<<addr;
			return;
		}
		std::cout <<"unaligned store8 address 0x"<<std::hex<<addr;
		exit(0);
	}
	///Store a 16bit (half a word)`val` into addr
	void Store16(uint32_t addr,uint16_t val)
	{
		if(addr%2!=0)
		{
			std::cout <<"unaligned store16 address 0x"<<std::hex<<addr;
			exit(0);
		}

		uint32_t abs_addr=map::mask_region(addr);

		if(uint32_t offset=map::SPU.contains(abs_addr))
		{
			std::cout <<"unhandeled write to SPU register"<<std::hex<<offset;
			return;
		}
		std::cout <<"unhandled store16 address 0x"<<std::hex<<addr;
		exit(0);
	}
	///Store a 32bit (aka a word) 'val'into 'addr' 
	void Store32(uint32_t addr,uint32_t val)
	{
		if((addr%4)	!=	0)
		{
			
			exit(0);
		}
		uint32_t abs_addr=map::mask_region(addr);
		if(uint32_t offset=map::MEMORYCONTROL.contains(abs_addr))
		{
			switch (offset)
			{
			case 0:
			{
				std::cout <<"bad expantion 1 address 0x"<<std::hex<<addr<<std::endl;
				//exit(0);
			}
			case 1:
			{
				std::cout <<"bad expantion 2 address 0x"<<std::hex<<addr<<std::endl;
				//exit(0);
			}
				break;
			
			default:
				std::cout <<"unhandeled write to MEMORYCONTROLE register"<<std::endl;
				break;
			}
			return;
		}
		std::cout <<"unhandeled store operation at address 0x"<<std::hex<<addr<<" with value 0x"<<std::hex<<val<<std::endl;
		//exit(0);
	}
};

struct Instruction
{
	//instruction code
	uint32_t OP;
	Instruction(){};
	Instruction(uint32_t opcode)
	{
		OP=opcode;
	}
	Instruction(const Instruction &ins){OP=ins.OP;}
	///the op code (6bit)
	uint32_t function()
	{
		return OP>>26;
	}
	///destination (5bit)
	RegisterIndex* t()
	{
		RegisterIndex* ri=new RegisterIndex((OP>>16)&0x1f);
		return ri;
	}
	///immediate value (16bit)
	uint32_t imm()
	{
		return OP&0xffff;
	}
	///sourse (5bit)
	RegisterIndex* s()
	{
		RegisterIndex* ri=new RegisterIndex((OP>>21)&0x1f);
		return	ri;
	}
	///immediate value signed (5bit)
	uint32_t imm_se()
	{
		int16_t v=(int16_t)(OP&0xffff);
		return (uint32_t)v;
	}
	///register index (5bit)
	RegisterIndex* d()
	{
		RegisterIndex* ri=new RegisterIndex((OP>>11)&0x1f);
		return ri;
	}
	///configuration for the instruction (6bit)
	uint32_t subfunction()
	{
		return OP&0x3f;
	}
	///shift immediate value (5bit)
	uint32_t shift()
	{
		return (OP>>6)&0x1f;
	}
	///immediate jump
	uint32_t imm_jump()
	{
		return (OP)&0x3FFFFFF;
	}
	uint32_t cop_opcode()
	{
		return (OP>>21)&0x1f;
	}
};

struct CPU
{
	/*CPU register*/
	uint32_t PC;//the famous programme counter
	std::array<uint32_t,32> regs;//32 register of the mips architecture
	//set of a the 32 regs to emulate the slot delea slot (seems that mips first generation was bad in handling it)
	//they will contain the output of the curent instruction
	std::array<uint32_t,32> regs_out;
	std::tuple<RegisterIndex*,uint32_t> load;
	/*CPU register*/

	/*coprocessor 0 register*/
	//register 12 status Register
	uint32_t sr;
	/*coprocessor 0 register*/

	//buses
	Interconnect* inter;
	//next instruction used to emulate the pipline behaveior
	Instruction* next_instruction;
	CPU(){};
	CPU(Interconnect* i)
	{
		PC=0xbfc00000;
		inter=i;
		for(int i=1;i<32;i++)
		{
			regs.at(i)=0xdeadbeef;
		}
		regs.at(0)=0;
		regs_out=regs;
		RegisterIndex* r0=new RegisterIndex(0);
		load={r0,0};
		next_instruction=new Instruction(0x0);
		sr=0;
	}
	uint32_t reg(RegisterIndex* index)
	{
		return regs.at((size_t)(index->index));
	}
	void set_reg(RegisterIndex* index,uint32_t val)
	{
		regs_out.at((size_t)(index->index))=val;
		regs_out.at(0)=0;
	}
	void store8(uint32_t addr,uint8_t val)
	{
		inter->Store8(addr,val);
	}
	void store16(uint32_t addr,uint16_t val)
	{
		inter->Store16(addr,val);
	}
	void stor32(uint32_t addr,uint32_t val)
	{
		inter->Store32(addr,val);
	}
	uint32_t Load32bit(uint32_t addr)
	{
		return inter->Load32bit(addr);
	}
	uint8_t Load8bit(uint32_t addr)
	{
		return inter->Load8bit(addr);
	}
	/*
	uint16_t Load16bit(uint32_t addr)
	{
		return inter->Load16bit(addr);
	}
	*/
	///branche to immediate value `offset` you know the boring stff if if else ...
	void branche(uint32_t offset)
	{
		offset =offset<<2;
		PC+=offset;
		//we added +4 in pipeline while fetching next instruction 
		//we compensat it with substructing it here
		PC-=4;
	}
	///load byte
	void op_lb(Instruction* ins)
	{
		uint32_t 		i	=	ins->imm_se();
		RegisterIndex* 	s	=	ins->s();
		RegisterIndex* 	t	=	ins->t();

		uint32_t		addr=	reg(s)+i;
		int8_t 	 		v=(int8_t)Load8bit(addr);
		load={t,(uint32_t)v};
	}
	///jump register
	void op_jr(Instruction* ins)
	{
		RegisterIndex* s=ins->s();
		std::cout <<"jr 0x"<<s->index;
		PC=reg(s);

	}
	///store byte
	void op_sb(Instruction* ins)
	{
		if(sr&0x10000!=0)
		{
			std::cout <<"ignoring store while cache is isolated";
			return;
		}
		int32_t 		i	=	ins->imm_se();
		RegisterIndex* 	s	=	ins->s();
		RegisterIndex* 	t	=	ins->t();

		uint32_t		addr=	reg(s)+i;
		uint32_t		v	=	reg(t) & 0xffff;
		std::cout <<"sb 0x"<<v<<",0x"<<i<<",0x"<<addr<<std::endl;
		
		store8(addr,(uint8_t)v);
	}
	///bitwise and immediate
	void op_andi(Instruction* ins)
	{
		uint32_t 		i	=	ins->imm();
		RegisterIndex* 	s	=	ins->s();
		RegisterIndex* 	t	=	ins->t();

		uint32_t		v	=	(reg(s))&i;
		std::cout<<"andi 0x"<<t->index<<",0x"<<s->index<<",0x"<<i<<std::endl;
		set_reg(t,v);
	}
	///Jump and link
	void op_jal(Instruction* ins)
	{
		regs_out.at(0x1f)=PC+4;
		std::cout <<"jal 0x"<<PC<<std::endl;
		std::cout<<"PC="<<PC<<std::endl;
		exit(0);
		op_j(ins);
	}
	///store halfword
	void op_sh(Instruction* ins)
	{
		if(sr&0x10000!=0)
		{
			std::cout <<"ignore store while caching";
			return;
		}
		uint32_t 		i	=	ins->imm_se();
		RegisterIndex* 	s	=	ins->s();
		RegisterIndex* 	t	=	ins->t();

		uint32_t		addr=	reg(s)+i;
		uint32_t		v	=	reg(t);
		std::cout<<"sh 0x"<<t->index<<",0x"<<i<<"(0x"<<s->index<<")";
		store16(addr,(uint16_t)v);
		
	}
	///add unsigned
	void op_addu(Instruction* ins)
	{
		RegisterIndex* d=ins->d();
		RegisterIndex* s=ins->s();
		RegisterIndex* t=ins->t();

		uint32_t v=reg(s)+reg(t);
		std::cout<<"addu 0x"<<d->index<<",0x"<<s->index<<",0x"<<t->index<<std::endl;
		set_reg(d,v);
	}
	///set on less than unsigned
	void op_sltu(Instruction* ins)
	{
		RegisterIndex* d=ins->d();
		RegisterIndex* s=ins->s();
		RegisterIndex* t=ins->t();

		bool v=reg(s)<reg(t);
		std::cout<<"stlu 0x"<<d->index<<",0x"<<s->index<<",0x"<<t->index<<std::endl;

		set_reg(d,(uint32_t)v);
	}
	///Load word
	void op_lw(Instruction* ins)
	{
		if(sr&0x10000!=0)
		{
			std::cout <<"Ignore_Load_cacheis_isolated";
			return;
		}

		int32_t 		i	=	ins->imm_se();
		RegisterIndex* 	s	=	ins->s();
		RegisterIndex* 	t	=	ins->t();
		
		uint32_t 		addr=	reg(s)+i;

		uint32_t		v	=	Load32bit(addr);
		std::cout<<"lw 0x"<<t->index<<",0x"<<i<<"(0x"<<s->index<<")"<<std::endl;
		//set_reg(t,v);
		load={t,v};
		
	}
	///add immediate and check for overflow (yes we all hate overflow :))
	void op_addi(Instruction* ins)
	{
		int32_t 		i	=	ins->imm_se();
		RegisterIndex* 	s	=	ins->s();
		RegisterIndex* 	t	=	ins->t();

		int32_t 		s1	=	(int32_t)reg(s);

		uint32_t		v;
		if(((s1+i)<INT32_MAX)&&((s1+i)>INT32_MIN))
		{
			v=(uint32_t)(s1+i);
			
		}
		else
		{
			exit(0);
		}
		std::cout <<"addi 0x"<<s->index<<",0x"<<t->index<<",0x"<<i<<std::endl;
		set_reg(t,v);	
	}
	///branch if not equals
	void op_bne(Instruction* ins)
	{
		int32_t 		i	=	ins->imm_se();
		RegisterIndex* 	s	=	ins->s();
		RegisterIndex* 	t	=	ins->t();
		if(reg(s)!=reg(t))
		{
			branche(i);
		}
		std::cout <<"bne 0x"<<s->index<<",0x"<<t->index<<",0x"<<(i<<2)<<std::endl;
		
	}
	///move to coprocesseur 0
	void op_mtc0(Instruction* ins)
	{
		RegisterIndex* 	cpu_r	=	ins->t()		;
		uint32_t 		cop_r	=	ins->d()->index	;

		uint32_t 		v		=	reg(cpu_r);

		switch (cop_r)
		{
		case 3|5|6|7|9|11:
			std::cout << "unhandeled_write_cop0_register";
			
			break;
		case 12:
			std::cout << "mtc0 0x"<<cpu_r->index<<",0x"<<cop_r;
			sr=v;
			
			break;
		case 13:
			if(v!=0)
			{
				std::cout <<"unhandled_write_to_cause_register";
				
			}
			break;
		default:
			std::cout << "unhandeled_cop0_register 0x" << std::hex<<cop_r<<std::endl;
			break;
		}
	}
	///coprocessor 0 opcode
	void op_cop0(Instruction* ins)
	{
		switch (ins->cop_opcode())
		{
		case 0b00100:
			op_mtc0(ins);
			break;
		
		default:
		{
			std::cout << "unhandeled_cop0_instruction 0x" << std::hex<< ins->OP<<std::endl;
			exit(0);
			break;
		}
		}
	}
	///bitwise or
	void op_or(Instruction* ins)
	{
		RegisterIndex* s=ins->s();
		RegisterIndex* t=ins->t();
		RegisterIndex* d=ins->d();

		uint32_t v=(	reg(s)	|	reg(t)	);
		std::cout <<"or 0x"<<d->index<<",0x"<<s->index<<",0x"<<t->index<<std::endl;
		set_reg(d,v);
	}
	///jump
	void op_j(Instruction* ins)
	{
		uint32_t jaddr=ins->imm_jump();
		
		PC=	(PC&0xf0000000)|(jaddr<<2);
		std::cout <<"j 0x"<<PC<<std::endl;
	}
	///add immediate unsigned
	void op_addiu(Instruction* ins)
	{
		uint32_t i=ins->imm_se();
		RegisterIndex* t=ins->t();
		RegisterIndex* s=ins->s();

		uint32_t v=reg(s)+i;
		std::cout<<"addiu 0x"<<t->index<<",0x"<<s->index<<",0x"<<i<<std::endl;
		
		set_reg(t,v);
	}
	///shift left logical
	void op_sll(Instruction* ins)
	{
		uint32_t i=ins->shift();
		RegisterIndex* t=ins->t();
		RegisterIndex* d=ins->d();

		uint32_t v=reg(t)<<i;
		std::cout <<"sll 0x"<<d->index<<",0x"<<t->index<<",0x"<<i<<std::endl;
		set_reg(d,v);
	}
	///Store word
	void op_sw(Instruction* ins)
	{
		uint32_t i=ins->imm_se();
		RegisterIndex* t=ins->t();
		RegisterIndex* s=ins->s();

		uint32_t addr=reg(s)+i;
		uint32_t v=reg(t);

		std::cout <<"sw 0x"<<t->index<<",0x"<<i<<"("<<s->index<<")"<<std::endl;
		
		
		stor32(addr,v);
	}
	///Bitwise or immediate
	void op_ori(Instruction* ins)
	{
		uint32_t i=ins->imm();
		RegisterIndex* t=ins->t();
		RegisterIndex* s=ins->s();

		uint32_t v=reg(s)|i;
		std::cout << "ori 0x"<<s->index<<",0x"<<t->index<<",0x"<<i<<std::endl;
		set_reg(t,v);
	}
	///Load upper immediate
	void op_lui(Instruction* ins)
	{
		uint32_t i=ins->imm();
		RegisterIndex* t=ins->t();
		
		uint32_t v=i<<16;
		std::cout<<"lui 0x"<<t->index<<",0x"<<v<<std::endl;
		set_reg(t,v);

	}
	void decode_and_execute(Instruction* Ins)
	{
		switch (Ins->function())
		{
		case 0b001111:
			op_lui(Ins);
			break;
		case 0b001101:
			op_ori(Ins);
			break;
		case 0b101011:
			op_sw(Ins);
			break;	
		case 0b000000:
			switch (Ins->subfunction())
			{
			case 0b000000:
				op_sll(Ins);
				break;
			case 0b100101:
				op_or(Ins);
				break;
			case 0b101011:
				op_sltu(Ins);
				break;
			case 0b100001:
				op_addu(Ins);
				break;
			case 0b001000:
				op_jr(Ins);
				break;
			default:
				std::cout <<"unhandled expression 0x"<<std::hex<<Ins->OP<<"0x"<<std::hex<<Ins->subfunction()<<std::endl;
				exit(0);
				break;
			}
			break;
		case 0b001001:
			op_addiu(Ins);
			break;
		case 0b010000:
			op_cop0(Ins);
			break;
		case 0b000010:
			op_j(Ins);
			break;
		case 0b000101:
			op_bne(Ins);
			break;
		case 0b001000:
			op_addi(Ins);
			break;
		case 0b100011:
			op_lw(Ins);
			break;
		case 0b101001:
			op_sh(Ins);
			break;
		case 0b000011:
			op_jal(Ins);
			break;
		case 0b101000:
			op_sb(Ins);
			break;
		case 0b001100:
			op_addi(Ins);
			break;
		case 0b100000:
			op_lb(Ins);
			break;
		default:
			std::cout <<"unhandled expression 08x"<<std::hex<<Ins->OP<<"/////0x"<<std::hex<<Ins->function()<<std::endl;
			exit(0);
			break;
		}
	}
	void run_next_ins()
	{
		RegisterIndex* tempRegIndex;
		uint32_t val;
		std::tie(tempRegIndex,val)=load;
		set_reg(tempRegIndex,val);
		load={new RegisterIndex(),0};
		Instruction* ins=next_instruction;
		next_instruction=new Instruction(Load32bit(PC));
		std::cout <<"PC="<<PC<<"0x"<<std::hex<<ins->OP<<"/0x"<<std::hex<<ins->function()<<"/0x"<<std::hex<<ins->subfunction()<<std::endl;
		PC+=4;
		decode_and_execute(ins);
		regs=regs_out;
	}
};
int main()
{

	BIOS* Bios=new BIOS("SCPH1001.bin");
	RAM*  R=new RAM();
	Interconnect* Inter=new Interconnect(Bios,R);
	CPU* cpu=new CPU(Inter);	
	for(;;){
		cpu->run_next_ins();
	}
	return 0;
}