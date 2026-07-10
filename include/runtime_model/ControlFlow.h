// ControlFlow.h
// Dylan Armstrong, 2026

#pragma once

// This is a control flow enum that determines the control flow inside a T# program
// NORMAL: proceed as per normal
// RETURN_VALUE: if something (e.g. a function or a method) needs to return a value of a given type - then the program (caller) should wait for that value to be returned
// BREAK: break from the current function (break statement in T#)
// CONTINUE: continue to the next step (continue statement in T#)
enum class ControlFlow { NORMAL,
						 RETURN_VALUE,
						 BREAK,
						 CONTINUE };