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

#ifndef LOBSTER_DISASM
#define LOBSTER_DISASM

#include "natreg.h"
#define FLATBUFFERS_DEBUG_VERIFICATION_FAILURE
#include "lobster/bytecode_generated.h"

namespace lobster {

inline string_view flat_string_view(const flatbuffers::String *s) {
    return string_view(s->c_str(), s->size());
}

inline string_view IdName(const bytecode::BytecodeFile *bcf, int i) {
    auto s = bcf->idents()->Get(bcf->specidents()->Get(i)->ididx())->name();
    return flat_string_view(s);
}

const bytecode::LineInfo *LookupLine(const int *ip, const int *code,
                                     const bytecode::BytecodeFile *bcf);

const int *DisAsmIns(NativeRegistry &natreg, ostringstream &ss, const int *ip, const int *code,
                     const type_elem_t *typetable, const bytecode::BytecodeFile *bcf);

void DisAsm(NativeRegistry &natreg, ostringstream &ss, string_view bytecode_buffer);

}  // namespace lobster

#endif  // LOBSTER_DISASM
