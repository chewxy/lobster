// Copyright 2014 Wouter van Oortmerssen. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "lobster/stdafx.h"
#include "lobster/disasm.h"

namespace lobster {

const bytecode::LineInfo *LookupLine(const int *ip, const int *code,
                                            const bytecode::BytecodeFile *bcf) {
    auto lineinfo = bcf->lineinfo();
    int pos = int(ip - code);
    int start = 0;
    auto size = lineinfo->size();
    assert(size);
    for (;;) {  // quick hardcoded binary search
        if (size == 1) return lineinfo->Get(start);
        auto nsize = size / 2;
        if (lineinfo->Get(start + nsize)->bytecodestart() <= pos) {
            start += nsize;
            size -= nsize;
        } else {
            size = nsize;
        }
    }
}

const int *DisAsmIns(NativeRegistry &natreg, ostringstream &ss, const int *ip, const int *code,
                            const type_elem_t *typetable, const bytecode::BytecodeFile *bcf) {
    auto ilnames = ILNames();
    auto li = LookupLine(ip, code, bcf);
    // FIXME: some indication of the filename, maybe with a table index?
    ss << "I " << int(ip - code) << " \tL " << li->line() << " \t";
    if (*ip < 0 || *ip >= IL_MAX_OPS) {
        ss << "ILLEGAL INSTRUCTION: " << *ip;
        return nullptr;
    }
    ss << ilnames[*ip] << ' ';
    int opc = *ip++;
    if (opc < 0 || opc >= IL_MAX_OPS) {
        ss << opc << " ?";
        return ip;
    }
    switch(opc) {
        case IL_PUSHINT:
        case IL_PUSHFUN:
        case IL_CONT1:
        case IL_JUMP:
        case IL_JUMPFAIL:
        case IL_JUMPFAILR:
        case IL_JUMPFAILN:
        case IL_JUMPNOFAIL:
        case IL_JUMPNOFAILR:
        case IL_LOGREAD:
        case IL_ISTYPE:
        case IL_EXIT:
        case IL_IFOR:
        case IL_VFOR:
        case IL_SFOR:
        case IL_NFOR:
        case IL_INCREF:
            ss << *ip++;
            break;

        case IL_PUSHINT64:
        case IL_PUSHFLT64: {
            auto v = Read64FromIp(ip);
            if (opc == IL_PUSHINT64) ss << v;
            else {
                int2float64 i2f;
                i2f.i = v;
                ss << i2f.f;
            }
            break;
        }

        case IL_LOGWRITE:
        case IL_KEEPREF:
            ss << *ip++ << ' ';
            ss << *ip++;
            break;

        case IL_RETURN: {
            auto id = *ip++;
            ip++;  // retvals
            ss << flat_string_view(bcf->functions()->Get(id)->name());
            break;
        }

        case IL_CALL:
        case IL_CALLMULTI: {
            auto bc = *ip++;
            assert(code[bc] == IL_FUNSTART || code[bc] == IL_FUNMULTI);
            auto id = code[bc + 1];
            auto nargs = code[bc + (opc == IL_CALLMULTI ? 1 : 0)];
            if (opc == IL_CALLMULTI) ip += nargs;  // arg types.
            ss << nargs << ' ' << flat_string_view(bcf->functions()->Get(id)->name());
            ss << ' ' << bc;
            break;
        }

        case IL_NEWVEC: {
            ip++;  // ti
            auto nargs = *ip++;
            ss << "vector " << nargs;
            break;
        }
        case IL_NEWSTRUCT: {
            auto ti = (TypeInfo *)(typetable + *ip++);
            ss << flat_string_view(bcf->structs()->Get(ti->structidx)->name());
            break;
        }

        case IL_BCALLRET0:
        case IL_BCALLRET1:
        case IL_BCALLRET2:
        case IL_BCALLRET3:
        case IL_BCALLRET4:
        case IL_BCALLRET5:
        case IL_BCALLRET6:
        case IL_BCALLREF0:
        case IL_BCALLREF1:
        case IL_BCALLREF2:
        case IL_BCALLREF3:
        case IL_BCALLREF4:
        case IL_BCALLREF5:
        case IL_BCALLREF6:
        case IL_BCALLUNB0:
        case IL_BCALLUNB1:
        case IL_BCALLUNB2:
        case IL_BCALLUNB3:
        case IL_BCALLUNB4:
        case IL_BCALLUNB5:
        case IL_BCALLUNB6: {
            int a = *ip++;
            ss << natreg.nfuns[a]->name;
            break;
        }

        #undef LVAL
        #define LVAL(N) case IL_VAR_##N:
            LVALOPNAMES
        #undef LVAL
        case IL_PUSHVAR:
            ss << IdName(bcf, *ip++);
            break;

        #define LVAL(N) case IL_FLD_##N: case IL_LOC_##N:
            LVALOPNAMES
        #undef LVAL
        case IL_PUSHFLD:
        case IL_PUSHFLDMREF:
        case IL_PUSHLOC:
            ss << *ip++;
            break;

        case IL_PUSHFLT:
            ss << *(float *)ip;
            ip++;
            break;

        case IL_PUSHSTR:
            EscapeAndQuote(flat_string_view(bcf->stringtable()->Get(*ip++)), ss);
            break;

        case IL_FUNSTART: {
            auto fidx = *ip++;
            ss << (fidx >= 0 ? flat_string_view(bcf->functions()->Get(fidx)->name()) : "__dummy");
            ss << "(";
            int n = *ip++;
            while (n--) ss << IdName(bcf, *ip++) << ' ';
            n = *ip++;
            ss << "=> ";
            while (n--) ss << IdName(bcf, *ip++) << ' ';
            ss << '[' << *ip++ << ']';  // keep
            n = *ip++;  // owned
            while (n--) ss << IdName(bcf, *ip++) << ' ';
            ss << ")";
            break;
        }

        case IL_CORO: {
            ss << *ip++;
            ip++;  // typeinfo
            int n = *ip++;
            for (int i = 0; i < n; i++) ss <<" v" << *ip++;
            break;
        }

        case IL_FUNMULTI: {
            ss << flat_string_view(bcf->functions()->Get(*ip++)->name()) << " (multi_start) ";
            auto n = *ip++;
            auto nargs = *ip++;
            ss << n << ' ' << nargs;
            ip += (nargs + 1) * n;
            break;
        }
    }
    return ip;
}

void DisAsm(NativeRegistry &natreg, ostringstream &ss, string_view bytecode_buffer) {
    auto bcf = bytecode::GetBytecodeFile(bytecode_buffer.data());
    assert(FLATBUFFERS_LITTLEENDIAN);
    auto code = (const int *)bcf->bytecode()->Data();  // Assumes we're on a little-endian machine.
    auto typetable = (const type_elem_t *)bcf->typetable()->Data();  // Same.
    auto len = bcf->bytecode()->Length();
    const int *ip = code;
    while (ip < code + len) {
        ip = DisAsmIns(natreg, ss, ip, code, typetable, bcf);
        ss << "\n";
        if (!ip) break;
    }
}

}  // namespace lobster
