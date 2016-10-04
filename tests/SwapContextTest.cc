/* Copyright (c) 2015-2016 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "gtest/gtest.h"
#include "Arachne.h"

static const size_t testStackSize = 256;
static char stack[testStackSize];
static void* stackPointer;
static void *oldStackPointer;

static bool firstEntryToSetContextTest;
static bool swapContextSuccess;

void setContextHelper() {
    firstEntryToSetContextTest = 0;

    register uint64_t rbx asm("rbx");
    register uint64_t r15 asm("r15");
    uint64_t foobar = rbx;
    EXPECT_EQ(*(reinterpret_cast<uint64_t*>(stackPointer) + 1), foobar);
    foobar = r15;
    EXPECT_EQ(*(reinterpret_cast<uint64_t*>(stackPointer) + 2), foobar);

    // This test is very much compiler dependent, because there is no way to
    // prevent the compiler from overwriting rbx and r15.
    asm("mov %0, %%rsp": "=g"(oldStackPointer));
    asm("retq");
}

void swapContextHelper() {
    swapContextSuccess = 1;
    Arachne::swapcontext(&oldStackPointer, &stackPointer);
}

TEST(SwapContextTest, SaveContext) {
    stackPointer = stack + testStackSize;
    EXPECT_EQ(256, reinterpret_cast<char*>(stackPointer) -
            reinterpret_cast<char*>(stack));
    asm("mov $1, %rbx\n\t"
        "mov $2, %r15\n\t"
        "mov $3, %r14\n\t"
        "mov $4, %r13\n\t"
        "mov $5, %r12\n\t");

    // This call is done using assembly because g++ will tend to clobber at
    // least one of the registers above if we use a call in C++
    // Arachne::savecontext(&stackPointer);
    asm("mov %0,%%edi": : "g"(&stackPointer));
    asm("callq %P0": : "i"(&Arachne::savecontext));

    EXPECT_EQ(208, reinterpret_cast<char*>(stackPointer) -
            reinterpret_cast<char*>(stack));
    EXPECT_EQ(1, *(reinterpret_cast<uint64_t*>(stackPointer) + 1));
    EXPECT_EQ(2, *(reinterpret_cast<uint64_t*>(stackPointer) + 2));
    EXPECT_EQ(3, *(reinterpret_cast<uint64_t*>(stackPointer) + 3));
    EXPECT_EQ(4, *(reinterpret_cast<uint64_t*>(stackPointer) + 4));
    EXPECT_EQ(5, *(reinterpret_cast<uint64_t*>(stackPointer) + 5));
}

TEST(SwapContextTest, SetContext) {
    stackPointer = stack + testStackSize;
    firstEntryToSetContextTest = 1;
    *reinterpret_cast<void**>(stackPointer) =
        reinterpret_cast<void*>(setContextHelper);
    Arachne::savecontext(&stackPointer);
    asm("mov %%r11, %0": "=g\n\t"(oldStackPointer));
    if (firstEntryToSetContextTest == 1)  {
        Arachne::setcontext(&stackPointer);
    }
    EXPECT_EQ(0, firstEntryToSetContextTest);
}

TEST(SwapContextTest, SwapContext) {
    swapContextSuccess = 0;
    stackPointer = stack + testStackSize;
    *reinterpret_cast<void**>(stackPointer) =
        reinterpret_cast<void*>(swapContextHelper);
    EXPECT_EQ(256, reinterpret_cast<char*>(stackPointer) -
            reinterpret_cast<char*>(stack));
    Arachne::savecontext(&stackPointer);
    EXPECT_EQ(208, reinterpret_cast<char*>(stackPointer) -
            reinterpret_cast<char*>(stack));
    Arachne::swapcontext(&stackPointer, &oldStackPointer);
    EXPECT_EQ(1, swapContextSuccess);
}