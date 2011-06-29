/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DFGRepatch.h"

#if ENABLE(DFG_JIT)

#include "DFGJITCodeGenerator.h"
#include "LinkBuffer.h"
#include "RepatchBuffer.h"

namespace JSC { namespace DFG {

static void dfgRepatchCall(CodeBlock* codeblock, CodeLocationCall call, FunctionPtr newCalleeFunction)
{
    RepatchBuffer repatchBuffer(codeblock);
    repatchBuffer.relink(call, newCalleeFunction);
}

static void dfgRepatchByIdSelfAccess(CodeBlock* codeBlock, StructureStubInfo& stubInfo, Structure* structure, size_t offset, const FunctionPtr &slowPathFunction, bool compact)
{
    RepatchBuffer repatchBuffer(codeBlock);

    // Only optimize once!
    repatchBuffer.relink(stubInfo.callReturnLocation, slowPathFunction);

    // Patch the structure check & the offset of the load.
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelPtrAtOffset(-(intptr_t)stubInfo.u.unset.deltaCheckImmToCall), structure);
    if (compact)
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.u.unset.deltaCallToLoadOrStore), sizeof(JSValue) * offset);
    else
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.u.unset.deltaCallToLoadOrStore), sizeof(JSValue) * offset);
}

static bool tryCacheGetByID(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    // FIXME: Write a test that proves we need to check for recursion here just
    // like the interpreter does, then add a check for recursion.

    CodeBlock* codeBlock = exec->codeBlock();
    JSGlobalData* globalData = &exec->globalData();
    
    if (isJSArray(globalData, baseValue) && propertyName == exec->propertyNames().length) {
        GPRReg baseGPR = static_cast<GPRReg>(stubInfo.baseGPR);
        GPRReg resultGPR = static_cast<GPRReg>(stubInfo.valueGPR);
        GPRReg scratchGPR = static_cast<GPRReg>(stubInfo.u.unset.scratchGPR);
        bool needToRestoreScratch = false;
        
        MacroAssembler stubJit;
        
        if (scratchGPR == InvalidGPRReg) {
            scratchGPR = JITCodeGenerator::selectScratchGPR(baseGPR, resultGPR);
            stubJit.push(scratchGPR);
            needToRestoreScratch = true;
        }
        
        MacroAssembler::Jump failureCase1 = stubJit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseGPR), MacroAssembler::TrustedImmPtr(globalData->jsArrayVPtr));
        
        stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSArray::storageOffset()), scratchGPR);
        stubJit.load32(MacroAssembler::Address(scratchGPR, OBJECT_OFFSETOF(ArrayStorage, m_length)), scratchGPR);
        MacroAssembler::Jump failureCase2 = stubJit.branch32(MacroAssembler::LessThan, scratchGPR, MacroAssembler::TrustedImm32(0));
        
        stubJit.orPtr(GPRInfo::tagTypeNumberRegister, scratchGPR, resultGPR);

        MacroAssembler::Jump success, fail;
        
        if (needToRestoreScratch) {
            stubJit.pop(scratchGPR);
            
            success = stubJit.jump();
            
            // link failure cases here, so we can pop scratchGPR, and then jump back.
            failureCase1.link(&stubJit);
            failureCase2.link(&stubJit);
            
            stubJit.pop(scratchGPR);
            
            fail = stubJit.jump();
        } else
            success = stubJit.jump();
        
        LinkBuffer patchBuffer(*globalData, &stubJit, codeBlock->executablePool());
        
        CodeLocationLabel slowCaseBegin = stubInfo.callReturnLocation.labelAtOffset(stubInfo.deltaCallToSlowCase);
        
        patchBuffer.link(success, stubInfo.callReturnLocation.labelAtOffset(stubInfo.deltaCallToDone));
        
        if (needToRestoreScratch)
            patchBuffer.link(fail, slowCaseBegin);
        else {
            // link failure cases directly back to normal path
            patchBuffer.link(failureCase1, slowCaseBegin);
            patchBuffer.link(failureCase2, slowCaseBegin);
        }
        
        CodeLocationLabel entryLabel = patchBuffer.finalizeCodeAddendum();
        stubInfo.stubRoutine = entryLabel;
        
        CodeLocationLabel hotPathBegin = stubInfo.hotPathBegin;
        RepatchBuffer repatchBuffer(codeBlock);
        repatchBuffer.relink(stubInfo.callReturnLocation.jumpAtOffset(stubInfo.deltaCallToStructCheck), entryLabel);
        repatchBuffer.relink(stubInfo.callReturnLocation, operationGetById);
        
        return true;
    }
    
    // FIXME: should support length access for String.

    // FIXME: Cache property access for immediates.
    if (!baseValue.isCell())
        return false;
    JSCell* baseCell = baseValue.asCell();
    Structure* structure = baseCell->structure();
    if (!slot.isCacheable())
        return false;
    if (structure->isUncacheableDictionary())
        return false;

    // Optimize self access.
    if (slot.slotBase() == baseValue) {
        if ((slot.cachedPropertyType() != PropertySlot::Value) || ((slot.cachedOffset() * sizeof(JSValue)) > (unsigned)MacroAssembler::MaximumCompactPtrAlignedAddressOffset))
            return false;

        dfgRepatchByIdSelfAccess(codeBlock, stubInfo, structure, slot.cachedOffset(), operationGetByIdBuildList, true);
        stubInfo.initGetByIdSelf(*globalData, codeBlock->ownerExecutable(), structure);
        return true;
    }
    
    // FIXME: should support prototype & chain accesses!
    return false;
}

void dfgRepatchGetByID(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    bool cached = tryCacheGetByID(exec, baseValue, propertyName, slot, stubInfo);
    if (!cached)
        dfgRepatchCall(exec->codeBlock(), stubInfo.callReturnLocation, operationGetById);
}

static bool tryBuildGetByIDList(ExecState* exec, JSValue baseValue, const Identifier&, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    if (!baseValue.isCell()
        || !slot.isCacheable()
        || baseValue.asCell()->structure()->isUncacheableDictionary()
        || slot.slotBase() != baseValue
        || slot.cachedPropertyType() != PropertySlot::Value
        || (slot.cachedOffset() * sizeof(JSValue)) > (unsigned)MacroAssembler::MaximumCompactPtrAlignedAddressOffset)
        return false;
    
    CodeBlock* codeBlock = exec->codeBlock();
    JSCell* baseCell = baseValue.asCell();
    Structure* structure = baseCell->structure();
    JSGlobalData* globalData = &exec->globalData();
    
    ASSERT(slot.slotBase().isObject());
    
    PolymorphicAccessStructureList* polymorphicStructureList;
    int listIndex = 1;
    
    if (stubInfo.accessType == access_get_by_id_self) {
        ASSERT(!stubInfo.stubRoutine);
        polymorphicStructureList = new PolymorphicAccessStructureList(*globalData, codeBlock->ownerExecutable(), stubInfo.callReturnLocation.labelAtOffset(stubInfo.deltaCallToSlowCase), stubInfo.u.getByIdSelf.baseObjectStructure.get());
        stubInfo.initGetByIdSelfList(polymorphicStructureList, 1);
    } else {
        polymorphicStructureList = stubInfo.u.getByIdSelfList.structureList;
        listIndex = stubInfo.u.getByIdSelfList.listSize;
    }
    
    if (listIndex < POLYMORPHIC_LIST_CACHE_SIZE) {
        stubInfo.u.getByIdSelfList.listSize++;
        
        GPRReg baseGPR = static_cast<GPRReg>(stubInfo.baseGPR);
        GPRReg resultGPR = static_cast<GPRReg>(stubInfo.valueGPR);
        
        MacroAssembler stubJit;
        
        MacroAssembler::Jump wrongStruct = stubJit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseGPR, JSCell::structureOffset()), MacroAssembler::TrustedImmPtr(structure));
        
        if (structure->isUsingInlineStorage())
            stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + slot.cachedOffset() * sizeof(JSValue)), resultGPR);
        else {
            stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::offsetOfPropertyStorage()), resultGPR);
            stubJit.loadPtr(MacroAssembler::Address(resultGPR, slot.cachedOffset() * sizeof(JSValue)), resultGPR);
        }
        
        MacroAssembler::Jump success = stubJit.jump();
        
        LinkBuffer patchBuffer(*globalData, &stubJit, codeBlock->executablePool());
        
        CodeLocationLabel lastProtoBegin = polymorphicStructureList->list[listIndex - 1].stubRoutine;
        ASSERT(!!lastProtoBegin);
        
        patchBuffer.link(wrongStruct, lastProtoBegin);
        patchBuffer.link(success, stubInfo.callReturnLocation.labelAtOffset(stubInfo.deltaCallToDone));
        
        CodeLocationLabel entryLabel = patchBuffer.finalizeCodeAddendum();
        
        polymorphicStructureList->list[listIndex].set(*globalData, codeBlock->ownerExecutable(), entryLabel, structure);
        
        CodeLocationJump jumpLocation = stubInfo.callReturnLocation.jumpAtOffset(stubInfo.deltaCallToStructCheck);
        RepatchBuffer repatchBuffer(codeBlock);
        repatchBuffer.relink(jumpLocation, entryLabel);
        
        if (listIndex < (POLYMORPHIC_LIST_CACHE_SIZE - 1))
            return true;
    }
    
    return false;
}

void dfgBuildGetByIDList(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    bool dontChangeCall = tryBuildGetByIDList(exec, baseValue, propertyName, slot, stubInfo);
    if (!dontChangeCall)
        dfgRepatchCall(exec->codeBlock(), stubInfo.callReturnLocation, operationGetById);
}

static V_DFGOperation_EJJI appropriatePutByIdFunction(const PutPropertySlot &slot, PutKind putKind)
{
    if (slot.isStrictMode()) {
        if (putKind == Direct)
            return operationPutByIdDirectStrict;
        return operationPutByIdStrict;
    }
    if (putKind == Direct)
        return operationPutByIdDirectNonStrict;
    return operationPutByIdNonStrict;
}

static bool tryCachePutByID(ExecState* exec, JSValue baseValue, const Identifier&, const PutPropertySlot& slot, StructureStubInfo& stubInfo, PutKind putKind)
{
    CodeBlock* codeBlock = exec->codeBlock();
    JSGlobalData* globalData = &exec->globalData();

    if (!baseValue.isCell())
        return false;
    JSCell* baseCell = baseValue.asCell();
    Structure* structure = baseCell->structure();
    if (!slot.isCacheable())
        return false;
    if (structure->isUncacheableDictionary())
        return false;

    // Optimize self access.
    if (slot.base() == baseValue) {
        if (slot.type() == PutPropertySlot::NewProperty)
            return false;

        dfgRepatchByIdSelfAccess(codeBlock, stubInfo, structure, slot.cachedOffset(), appropriatePutByIdFunction(slot, putKind), false);
        stubInfo.initPutByIdReplace(*globalData, codeBlock->ownerExecutable(), structure);
        return true;
    }

    // FIXME: should support the transition case!
    return false;
}

void dfgRepatchPutByID(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PutPropertySlot& slot, StructureStubInfo& stubInfo, PutKind putKind)
{
    bool cached = tryCachePutByID(exec, baseValue, propertyName, slot, stubInfo, putKind);
    if (!cached)
        dfgRepatchCall(exec->codeBlock(), stubInfo.callReturnLocation, appropriatePutByIdFunction(slot, putKind));
}

} } // namespace JSC::DFG

#endif
