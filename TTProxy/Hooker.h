#ifndef _YEBISOCKS_HOOKER_H_
#define _YEBISOCKS_HOOKER_H_

#include <YCL/Hashtable.h>
using namespace yebisuya;

class Hooker {
private:
    template<class TYPE>
	class Ptr {
	public:
		static TYPE* cast(void* p, int offset) {
			return (TYPE*) ((LPBYTE) p + offset);
		}
    };
    static bool modifyModuleProc(HMODULE module, FARPROC oldproc, FARPROC newproc) {
		IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*) module;
		if (::IsBadReadPtr(dos, sizeof *dos) || dos->e_magic != IMAGE_DOS_SIGNATURE) {
			return false;
		}
		
		IMAGE_NT_HEADERS* nt = Ptr<IMAGE_NT_HEADERS>::cast(dos, dos->e_lfanew);
		if (::IsBadReadPtr(nt, sizeof *nt) || nt->Signature != IMAGE_NT_SIGNATURE) {
			return false;
		}
		
		IMAGE_IMPORT_DESCRIPTOR* desc = Ptr<IMAGE_IMPORT_DESCRIPTOR>::cast(module, nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		if (desc == (IMAGE_IMPORT_DESCRIPTOR*) nt) {
			return false;
		}
		
		DWORD o;
		for (; desc->Name; desc++){
			for (IMAGE_THUNK_DATA* thunk = Ptr<IMAGE_THUNK_DATA>::cast(module, desc->FirstThunk); thunk->u1.Function; thunk++){
				if ((LPDWORD) thunk->u1.Function == (LPDWORD) oldproc) {
					if (::VirtualProtect(&thunk->u1.Function, sizeof thunk->u1.Function, PAGE_READWRITE, &o)){
						(LPDWORD&) thunk->u1.Function = (LPDWORD) newproc;
						::VirtualProtect(&thunk->u1.Function, sizeof thunk->u1.Function, o, &o);
						return true;
					}
				}
			}
		}
		return false;
    }

	static Hooker& instance() {
		static Hooker instance;
		return instance;
	}

	Hashtable<FARPROC, FARPROC> proctable;

	Hooker() {
	}
public:
	static void hook(HMODULE module, FARPROC oldproc, FARPROC newproc) {
		modifyModuleProc(module, oldproc, newproc);
	}
	static void setup(FARPROC oldproc, FARPROC newproc) {
		instance().proctable.put(oldproc, newproc);
	}
	static void hookAll(HMODULE module) {
		Pointer< Enumeration<FARPROC> > oldprocs = instance().proctable.keys();
		while (oldprocs->hasMoreElements()) {
			FARPROC oldproc = oldprocs->nextElement();
			FARPROC newproc = instance().proctable.get(oldproc);
			modifyModuleProc(module, oldproc, newproc);
		}
	}
	static void unhookAll(HMODULE module) {
		Pointer< Enumeration<FARPROC> > oldprocs = instance().proctable.keys();
		while (oldprocs->hasMoreElements()) {
			FARPROC oldproc = oldprocs->nextElement();
			FARPROC newproc = instance().proctable.get(oldproc);
			modifyModuleProc(module, newproc, oldproc);
		}
	}
	static FARPROC find(FARPROC original) {
		return instance().proctable.get(original);
	}
};

#endif//_YEBISOCKS_HOOKER_H_
