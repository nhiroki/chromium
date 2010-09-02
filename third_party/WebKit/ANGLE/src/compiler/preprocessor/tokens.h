/****************************************************************************\
Copyright (c) 2002, NVIDIA Corporation.

NVIDIA Corporation("NVIDIA") supplies this software to you in
consideration of your agreement to the following terms, and your use,
installation, modification or redistribution of this NVIDIA software
constitutes acceptance of these terms.  If you do not agree with these
terms, please do not use, install, modify or redistribute this NVIDIA
software.

In consideration of your agreement to abide by the following terms, and
subject to these terms, NVIDIA grants you a personal, non-exclusive
license, under NVIDIA's copyrights in this original NVIDIA software (the
"NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from the
NVIDIA Software without specific prior written permission from NVIDIA.
Except as expressly stated in this notice, no other rights or licenses
express or implied, are granted by NVIDIA herein, including but not
limited to any patent rights that may be infringed by your derivative
works or by other works in which the NVIDIA Software may be
incorporated. No hardware is licensed hereunder. 

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN ANY WAY
OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE
NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT,
TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF
NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
//
// tokens.h
//

#if !defined(__TOKENS_H)
#define __TOKENS_H 1

#include "compiler/preprocessor/parser.h"

#define EOF_SY (-1)

typedef struct TokenBlock_Rec TokenBlock;

typedef struct TokenStream_Rec {
    struct TokenStream_Rec *next;
    char *name;
    TokenBlock *head;
    TokenBlock *current;
} TokenStream;

struct TokenBlock_Rec {
    TokenBlock *next;
    int current;
    int count;
    int max;
    unsigned char *data;
};

extern TokenStream stdlib_cpp_stream;


TokenStream *NewTokenStream(const char *name, MemoryPool *pool);
void DeleteTokenStream(TokenStream *pTok); 
void RecordToken(TokenStream *pTok, int token, yystypepp * yylvalpp);
void RewindTokenStream(TokenStream *pTok);
int ReadToken(TokenStream *pTok, yystypepp * yylvalpp);
int ReadFromTokenStream(TokenStream *pTok, int name, int (*final)(CPPStruct *));
void UngetToken(int, yystypepp * yylvalpp);

#if defined(CPPC_ENABLE_TOOLS)

void DumpTokenStream(FILE *, TokenStream *, yystypepp * yylvalpp);

#endif // defined(CPPC_ENABLE_TOOLS)

#endif // !defined(__TOKENS_H)
