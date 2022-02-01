/*
 Modern clean-Sheet Instruction Set (McSIS):
    Stores memory by key/index pairs
    Highly flexible 64-bit instruction set
 Developed by CS 601 class 2022-01
*/
#include <sstream>
#include <iostream>
#include <map>
#include <ostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <string>
#include <iterator>
#include <stdexcept>


class McSIS {
public:
	typedef signed long long word; // 64-bit type
	typedef word key;  // area of memory
	typedef word index;  // location in memory
	bool stop;
	
	class keyindex {
	public:
		key k;
		index x;
		keyindex(key k_,index x_) { k=k_; x=x_; }
		bool operator<(const keyindex &o) const {
			if (k<o.k) return true;
			if (k>o.k) return false;
			if (x<o.x) return true;
			if (x>o.x) return false;
			return false; // <- actually equal!
		}
	};
	
	// HACK: this isn't a hashtable yet, don't touch directly so we can fix it later!
	typedef std::map<keyindex,word> hashtable_storage_t;
	hashtable_storage_t hashtable_storage;
	
	enum { nregisters=16};
	word registers[nregisters];
	// Register numbers used as index in key 0
	enum {
		not_a_register=-1,
		constant0=0,
		r1=1,
		r2=2,
        r3=3,
        r4=4,
        r5=5,
        r6=6,
        r7=7,
		PX=8, // program index
		PK=9, // program key
		AX=0xA, // src operand 1 index
		BX=0xB, // src2 index
		DK=0xC, // destination key
		DX=0xD, // destination index
		AK=0xE, // src1 key
		BK=0xF, // src2 key
	};
	
	// opcode
	enum {
		op_add=0xff,
		op_sub=0xfe
	};
	enum {
		comp_eq=0xE,
        comp_lt=2,
		
	};
const char *register_name[nregisters]={
"$0", 
"r1", 
"r2", 
"r3", 
"r4", 
"r5", 
"r6", 
"r7", 
"PX", 
"PK", 
"AX", 
"BX", 
"DK", 
"DX", 
"AK", 
"BK", 
};

enum {n_op=16};
const char * compare_op_name[n_op]={
"   "," < "," 2?"," 3?",
" 4?"," 5?"," 6?"," 7?",
" 8?"," 9?"," A?"," B?",
" C?"," D?","=="," F?",
};
	
	
	// Storage access, either internal or external
	inline word & hashtable(const key &k,const index &x) 
	{
		if (k==0) return registers[x&0xF];
		return hashtable_storage[keyindex(k,x)];
	}
	
	// Defines how operands are read
	word read_operand(word K,word X)
	{
		if (K==0) return registers[X]; // register access
		if (K==8) return X; // constant
		return hashtable(registers[K],registers[X]);
	}
	word &write_operand(word K,word X)
	{
		if (K==0) return registers[X]; // register access
		if (K==8) illegal("write to constant?!"); 
		return hashtable(registers[K],registers[X]);
	}
	
	// Execute one instruction
	void runi(const word &inst)
	{
		word opcode = inst & 0xff; 
		
		word cond = (inst>>32)&0xFFF;
		if (cond!=0) {
			word A  = (cond>>8)&0xF;
			word op = (cond>>4)&0xF;
			word B  = (cond>>0)&0xF;
			
			if (op==0xE) if (registers[A] != registers[B]) return; // skip instruction
			if (op==0x1) if (registers[A] >= registers[B]) return; // skip instruction
		}
		
		word overrides = inst>>8; 
		word 
			oDK=(overrides>>20)&0xF,
			oDX=(overrides>>16)&0xF,
			oAK=(overrides>>12)&0xF,
			oAX=(overrides>>8)&0xF,
			oBK=(overrides>>4)&0xF,
			oBX=(overrides>>0)&0xF;
			
		word A = read_operand(oAK,oAX);
		word B = read_operand(oBK,oBX);
		word &D = write_operand(oDK,oDX);
		if (opcode==op_add) { // add
			D = A+B;
		}
		else if (opcode==op_sub) { // sub
			D = A-B;
		}
		else
			illegal("not an instruction");
	}

	// Run the simulator
	word run()
	{
		while (!stop && --leash>0) {
			word fetch=hashtable(registers[PK],registers[PX]++);
			if (fetch==0) break;
			runi(fetch);
		}
		if (leash<=0) illegal("ran too long");
		
		return registers[1];
	}

	word leash; // instructions remaining to execute
	// Create a simulator with a block of program machine code
	McSIS(const word code[], word leash_=100) 
		:registers{0}
	{
		stop=false;
		leash=leash_;
		set_program(code);
	}
	
	// Upload a block of machine code in to the machine's execution area (PK/PX)
	void set_program(const word code[])
	{
		registers[PK]=0xC0DE;
		registers[PX]=0;
		for (int i=0;;i++) {
			hashtable(registers[PK],i)=code[i];
			if (code[i]==0) break; //<- zero terminate your programs!
		}
	}
	

	// Debug support: dump register values onscreen (in hex)
	void dump_registers(std::ostream &out=std::cout) 
	{
		for (int r=0;r<16;r++)
			out<<register_name[r]<<"="<<std::hex<<registers[r]<<" ";
        out<<std::endl;
	}
	
	// When CPU hit an illegal operation:
	int illegal(std::string why) {
		std::cout<<"FATAL> "<<why<<"\n";
		dump_registers();
		stop=true;
		throw std::runtime_error(why);
		return -1;
	}
	
// Assembly support:
// Assembly support:
	// Return a register, or not_a_register
	int assemble_register_or_not(std::string operand)
	{
		for (int r=0;r<nregisters;r++)
			if (operand==register_name[r])
				return r;
		return not_a_register;
	}
	
	// Return a register, or error illegal
	int assemble_register_for_X(std::string operand)
	{
		int r=assemble_register_or_not(operand);
		if (r!=not_a_register)
			return r;
		return illegal("Not a register: "+operand);
	}
	// Return a register, or error illegal
	int assemble_register_for_K(std::string operand)
	{
		int r=assemble_register_or_not(operand);
		if (r!=not_a_register)
		{
			if (r==constant0) return illegal("Can't use $0 as a key (key 0 means register access)");
			if (r==PX) return illegal("Can't use PX as a key (this means a constant)");
			return r;
		}
		return illegal("Not a register: "+operand);
	}
	word assemble_operand(std::string operand)
	{
		// Check if it's a register (0/ encoding)
		for (int r=0;r<nregisters;r++)
			if (operand==register_name[r])
				return (0x0<<4)+r;
		// Check if it's a constant ($ encoding)
		if (operand[0]=='$') {
			operand=operand.erase(0,1); // remove the $, leave the number
			int value=std::stoi(operand,0,10);
			if (value>=0 && value<16)
				return (0x8<<4)+value;
			else
				illegal("can't assemble "+operand);
		}
		// Check if it's a hashtable access
		if (operand[0]=='h'){
			auto hashref=operand.substr(0,10);
			if (hashref!="hashtable[")illegal("Can't parse "+operand+": missing hashtable?");
			std::string k=operand.substr(10,2);
			char slash=operand[12];
			if (slash!='/') illegal("Can't parse "+operand+": missing slash?");
			std::string x=operand.substr(13,2);
			char close=operand[15];
			if (close!=']') illegal("Can't parse "+operand+": missing close?");
			return (assemble_register_for_K(k)<<4)+(assemble_register_for_X(x));
        if (operand == "==")
            return (0x4<<4)+comp_eq;
        else
            return (illegal("Unknown operand type "+operand));
		return 0;
		}
	}
	
	// Remove trailing comma from string
	std::string decomma(std::string s)
	{
		if (s=="") return s;
		if (s.back()==',') {
			s.pop_back();
			return s;
		}
		return s;
	}

	word assemble_instruction(std::string line)
	{
        std::string nested = "";
		std::istringstream in(line);
		word inst=0;
        word highbits = inst>>32;
        word lowbits = inst & 0xffffffff;
		std::string opcode; in>>opcode;
        if (opcode == "exit"){
            return inst;
        }
        if (opcode[0] == 'i'){
		    for (auto i=0; i < 6; i++){
                std::string D; in>>D; D=decomma(D);
                nested = nested + D + " ";

            }
            lowbits = assemble_instruction(nested);
            auto start = opcode.find("(");
            auto end = opcode.find(")");
            auto new_opcode = opcode.substr(start + 1, (end - start)-1 );
            auto comp1 = new_opcode.substr(0,2);
            std::string operand;
            std::string comp2;
            if (new_opcode.length() == 6){
                operand = new_opcode.substr(2,2);
                comp2 = new_opcode.substr(4,2);
            }
            else if (new_opcode.length() == 5){
                operand = new_opcode.substr(2,1);
                comp2 = new_opcode.substr(3,2);
                
            }
            highbits = highbits | assemble_operand(comp1)<<32;
            highbits = highbits | assemble_operand(operand)<<36;
            highbits = highbits | assemble_operand(comp2)<<40;
            inst = highbits | lowbits;
            return inst;
            illegal("we cant handle smaller right now....");
        }
		std::string D; in>>D; D=decomma(D);
		std::string A; in>>A; A=decomma(A);
		std::string B; in>>B; B=decomma(B);
		
		if (opcode=="add") inst+=op_add;
		if (opcode=="mov") {
			inst+=op_add;
			B=A;
			A="$0";
		}
        if (opcode=="sub") inst+=op_sub;

		// Add the overrides to the instruction:
		inst = inst | assemble_operand(D)<<24;
		inst = inst | assemble_operand(A)<<16;
		inst = inst | assemble_operand(B)<<8;
		
		return inst;
	}
	
// Disassembly support:
	void disassemble_operand(int K,int X,std::ostream &out=std::cout)
	{
		if (K==0) out<<register_name[X]<<""; // register access
		else if (K==8) out<<"$"<<X; // constant
		else  out<<"hashtable["<<register_name[K]<<","<<register_name[X]<<"]";
	}
	void disassemble_instruction(word inst,std::ostream &out=std::cout)
	{
		//out<<std::hex<<std::setw(16)<<inst<<"   ";
		//out<<std::hex<<std::setfill('0')<<std::setw(16)<<inst<<"   ";
		word highbits = inst>>32;
		word lowbits = inst & 0xffffffff;
		out<<std::hex<<std::setfill('0');
		if (highbits)
			out<<"0x"<<std::setw(8)<<highbits;
		else
			out<<"        0x";
		out<<std::setfill('0')<<std::setw(8)<<lowbits<<"   ";
		
		word opcode = inst & 0xff; 
		
		word cond = (inst>>32)&0xFFF;
		if (cond!=0) {
			word A  = (cond>>8)&0xF;
			word op = (cond>>4)&0xF;
			word B  = (cond>>0)&0xF;
			out<<"if("<<register_name[A]<<compare_op_name[op]<<register_name[B]<<") ";
		}
		
		word overrides = inst>>8; 
		word 
			oDK=(overrides>>20)&0xF,
			oDX=(overrides>>16)&0xF,
			oAK=(overrides>>12)&0xF,
			oAX=(overrides>>8)&0xF,
			oBK=(overrides>>4)&0xF,
			oBX=(overrides>>0)&0xF;
			
		bool second_operand=true;
		if (opcode==op_add) {
			if (oAK==0 && oAX==0)
			{ // special case for mov:
				out<<"mov ";
				second_operand=false;
			}
			else
			{
				out<<"add ";
			}
			
		}
        else if (opcode == op_sub){
                out<<"sub ";
        }
		else out<<"opcode["<<opcode<<"] ";
		
		disassemble_operand(oDK,oDX,out);
		out<<", ";
		if (second_operand) {
			disassemble_operand(oAK,oAX,out);
			out<<", ";
		}
		disassemble_operand(oBK,oBX,out);
		out<<std::endl;
	}
	void disassemble(std::ostream &out=std::cout)
	{
		long px=registers[PX];
		while (true) {
			out<<std::hex<<std::setfill(' ')<<std::setw(2)<<px<<": ";
			word fetch=hashtable(registers[PK],px++);
			if (fetch==0) break;
			
			disassemble_instruction(fetch,out);
		}
	}
	void print_program(word inst, std::ostream &out=std::cout){
		word highbits = inst>>32;
		word lowbits = inst & 0xffffffff;
		out<<std::hex<<std::setfill('0');
		if (highbits)
			out<<"0x"<<std::setw(8)<<highbits;
		else
			out<<"        0x";
		out<<std::setfill('0')<<std::setw(8)<<lowbits<<"   ";
		out<<std::endl;
	}
};



long foo(std::vector<std::string> & tester)
{
	
	McSIS::word program[]={
		0x010081ff,
		0x0 // terminating zero
	};
	McSIS m(program);
    for (auto i:tester){
        m.print_program(m.assemble_instruction(i));
    }
	long v=m.run();
	return v;
}

int main(void){
    std::vector<std::string> program;
    std::ifstream file_in("test.txt");
    if (!file_in) {std::cout << "error";}
    std::string line;
    while (std::getline(file_in, line))
    {
        program.push_back(line);
    }
    long res = foo(program);
    std::cout  << res << std::endl;
    return 1;
}