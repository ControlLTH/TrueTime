/*
 * Copyright (c) 2016 Lund University
 *
 * Written by Anton Cervin, Dan Henriksson and Martin Ohlin,
 * Department of Automatic Control LTH, Lund University, Sweden.
 *   
 * This file is part of TrueTime 2.0.
 *
 * TrueTime 2.0 is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * TrueTime 2.0 is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TrueTime 2.0. If not, see <http://www.gnu.org/licenses/>
 */

#ifndef __TRUETIME_H__
#define __TRUETIME_H__

//#define TTDEBUG    // Uncomment this line to turn on debug printfs

// Some Visual Studio C++ defines and includes
#ifdef _MSC_VER 
#define snprintf _snprintf
#define fmax(a,b) ((a)>(b)?(a):(b))
#define fmin(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#include <string.h>
#include <math.h>

#include <algorithm>  // for std::min and std::max
using namespace std;

#define MAXCHARS 100  // Maximum length of user-defined names
#define MAXERRBUF 500 // Maximum length of formatted error messages

#include "mex.h"
#include "matrix.h"

#include "mexhelp.h"
#include "linkedlist.cpp"
#include "datanode.h"

#define TT_TIME_RESOLUTION 1.0E-12  // Maximum timing precision of TrueTime
#define TT_MAX_TIMESTEP    1000.0   // Default timestep if no pending event

#define TT_MAX_ITER        1000     // Maximum kernel iterations at one timestep

//#define max(A,B) ((A) > (B) ? (A) : (B))
//#define min(A,B) ((A) < (B) ? (A) : (B))

#ifdef TTDEBUG
#define DEBUGG 1
#else
#define DEBUGG 0
#endif
#define debugPrintf(...) \
   do { if (DEBUGG) { mexPrintf(__VA_ARGS__); mexEvalString("pause(.001);"); }} while (0)
#endif
