#include <iostream>
#include <map>
#include <ostream>

class McSis {
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
	
	word registers[16];
	// Register numbers used as index in key 0
	enum {
		r1=1,
		r2=2,
		PX=8, // program index
		PK=9, // program key
		AX=0xA, // src operand 1 index
		BX=0xB, // src2 index
		DK=0xC, // destination key
		DX=0xD, // destination index
		AK=0xE, // src1 key
		BK=0xF, // src2 key
	};
	
	// Storage access
	inline word & hashtable(const key &k,const index &x) 
	{
		if (k==0) return registers[x&0xF];
		return hashtable_storage[keyindex(k,x)];
	}
	
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
		
		word cond = (inst>>32)&0xF;
		if (cond==0x8 && registers[1]>40) return;
		
		word overrides = inst>>8; 
		word 
			oDK=(overrides>>20)&0xF,
			oDX=(overrides>>16)&0xF,
			oAK=(overrides>>12)&0xF,
			oAX=(overrides>>8)&0xF,
			oBK=(overrides>>4)&0xF,
			oBX=(overrides>>0)&0xF;
		if (opcode==0xFF) { // add
			word A = read_operand(oAK,oAX);
			word B = read_operand(oBK,oBX);
			word &D = write_operand(oDK,oDX);
			D = A+B;
		}
		else
			illegal("not an instruction");
	}
	
	// When CPU hit an illegal operation:
	void illegal(const char *why) {
		std::cout<<"FATAL> "<<why<<"\n";
		stop=true;
	}
	
	word leash; // instructions remaining to execute
	McSis(const word code[], word leash_=100) 
		:registers{0}
	{
		stop=false;
		registers[PK]=0xC0DE;
		registers[PX]=0;
		for (int i=0;;i++) {
			hashtable(registers[PK],i)=code[i];
			if (code[i]==0) break; //<- zero terminate your programs, fool!
		}
		leash=leash_;
	}
	
	word run()
	{
		while (!stop && --leash>0) {
			word fetch=hashtable(registers[PK],registers[PX]++);
			if (fetch==0) break;
			runi(fetch);
		}
		
		return registers[1];
	}
};



long foo(McSis::word const program[])
{
	McSis m(program);
	return m.run();
}

int main(){
    McSis::word program[]={
		0x028F8FFF, // [0] r2 = F+F;
		0x0D0086FF, // [1] DX = 6
		0x0C0081FF, // [2] DK = 1
		0xCD0002FF, // [3] DK/DX = r2
		0x0100CDFF, // [4] r1 = DK/DX
		0x010181FF, // [5] r1++
		0x8080085FF, // [6] PX = const 5 (jump to line!)
		//0x010098FF, // [7] r1 = the next constant!
		//700,
		0x0 // terminating zero
	};
    long res = foo(program);
    std::cout << res << std::endl;
    return 1;
}