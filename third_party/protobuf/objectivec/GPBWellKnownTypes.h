// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import <Foundation/Foundation.h>

#import "google/protobuf/Any.pbobjc.h"
#import "google/protobuf/Duration.pbobjc.h"
#import "google/protobuf/Timestamp.pbobjc.h"


NS_ASSUME_NONNULL_BEGIN

// Extension to GPBTimestamp to work with standard Foundation time/date types.
@interface GPBTimestamp (GBPWellKnownTypes)
@property(nonatomic, readwrite, strong) NSDate *date;
@property(nonatomic, readwrite) NSTimeInterval timeIntervalSince1970;
- (instancetype)initWithDate:(NSDate *)date;
- (instancetype)initWithTimeIntervalSince1970:(NSTimeInterval)timeIntervalSince1970;
@end

// Extension to GPBDuration to work with standard Foundation time type.
@interface GPBDuration (GBPWellKnownTypes)
@property(nonatomic, readwrite) NSTimeInterval timeIntervalSince1970;
- (instancetype)initWithTimeIntervalSince1970:(NSTimeInterval)timeIntervalSince1970;
@end

// Extension to GPBAny to support packing and unpacking for arbitrary messages.
@interface GPBAny (GPBWellKnownTypes)
// Initialize GPBAny instance with the given message. e.g., for google.protobuf.foo, type
// url will be "type.googleapis.com/google.protobuf.foo" and value will be serialized foo.
- (instancetype)initWithMessage:(GPBMessage*)message;
// Serialize the given message to the value in GPBAny. Type url will also be updated.
- (void)setMessage:(GPBMessage*)message;
// Parse the value in GPBAny to the given message. If messageClass message doesn't match the
// type url in GPBAny, nil is returned.
- (GPBMessage*)messageOfClass:(Class)messageClass;
// True if the given type matches the type url in GPBAny.
- (BOOL)wrapsMessageOfClass:(Class)messageClass;
@end

// Common prefix of type url in GPBAny, which is @"type.googleapis.com/". All valid
// type urls in any should start with this prefix.
extern NSString *const GPBTypeGoogleApisComPrefix;

NS_ASSUME_NONNULL_END
